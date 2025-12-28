#include "engine.h"
#include "types.h"
#include "const_color_vs.h"
#include "const_color_ps.h"

#include <iostream>
#include <fstream>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

Engine::Engine() {}

Engine::Engine(HWND hwnd) : hwnd(hwnd) {
#ifdef _DEBUG
    // Enable the D3D12 debug layer.
    ID3D12Debug* debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->Release();
    } else {
        std::cout << "Failed to enable debug interface\n";
    }
#endif
    prepareForRendering();
}

Engine::~Engine() {
    if (fenceEvent) {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }
}

int Engine::getFrameIdx() {
    return frameIdx;
}

void Engine::prepareForRendering() {
    createDevice();

    createCommandsManagers();

    createSwapChain();
    createRenderTargetView();

    createBarriers();
    createFence();

    createVertexBuffer();
    createRootSignature();
    createPipelineState();
    createVpAndSc();
}

void Engine::createDevice() {
    HRESULT hr;

    hr = CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create device factory");
    }

    for (UINT i = 0; ; ++i) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        if (dxgiFactory->EnumAdapters1(i, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        hr = D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(device.GetAddressOf())
        );

        if (SUCCEEDED(hr)) {
            std::wcout << L"Using: " << desc.Description << std::endl;
            break;
        }
    }


    if (device.Get() == nullptr) {
        throw std::runtime_error("failed to create device");
    }
}

void Engine::createCommandsManagers() {
    HRESULT hr;

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to crerate command queue");
    }

    for (UINT i = 0; i < bufferCount; ++i) {
        hr = device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(commandAllocator[i].GetAddressOf()));
        if (FAILED(hr)) {
            throw std::runtime_error("failed to crerate command allocator");
        }

        hr = device->CreateCommandList(0, queueDesc.Type, commandAllocator[i].Get(), nullptr, IID_PPV_ARGS(commandList[i].GetAddressOf()));
        if (FAILED(hr)) {
            throw std::runtime_error("failed to crerate command list");
        }
        hr = commandList[i]->Close();
        if (FAILED(hr)) {
            throw std::runtime_error("failed to close command list");
        }
    }
}

void Engine::createSwapChain() {
    HRESULT hr;

    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        swapChain1.GetAddressOf()
    );
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create swap chain");
    }
    dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    hr = swapChain1.As(&swapChain);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to query IDXGISwapChain3");
    }
}

void Engine::createRenderTargetView() {
    HRESULT hr;

    D3D12_DESCRIPTOR_HEAP_DESC descrHeapDesc{};
    descrHeapDesc.NumDescriptors = bufferCount;
    descrHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    hr = device->CreateDescriptorHeap(&descrHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create descriptor heap");
    }

    UINT rtvStride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < bufferCount; ++ i) {
        hr = swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].GetAddressOf()));
        if (FAILED(hr)) {
            throw std::runtime_error("failed to get back buffer");
        }
        device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);

        this->rtvHandle[i] = rtvHandle;
        rtvHandle.ptr += rtvStride;
    }
}

void Engine::createBarriers() {
    for (UINT i = 0; i < bufferCount; i++) {
        presentToRTVBarrier[i] = {};
        presentToRTVBarrier[i].Transition.pResource   = backBuffers[i].Get();
        presentToRTVBarrier[i].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        presentToRTVBarrier[i].Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

        rtvToPresentBarrier[i] = {};
        rtvToPresentBarrier[i].Transition.pResource   = backBuffers[i].Get();
        rtvToPresentBarrier[i].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        rtvToPresentBarrier[i].Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    }
}

void Engine::createFence() {
    HRESULT hr;

    for (UINT i = 0; i < bufferCount; i++) {
        hr = device->CreateFence(fenceValue[i], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence[i].GetAddressOf()));
        if (FAILED(hr)) {
            throw std::runtime_error("failed to create fence");
        }
    }

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr) {
        throw std::runtime_error("failed to create fence event");
    }
}

void Engine::createVertexBuffer() {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc{};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = sizeof(triangleVerticies);
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(vertexBuffer.GetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create vertex buffer");
    }

    void* mapped = nullptr;
    D3D12_RANGE readRange{0, 0};
    hr = vertexBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr)) throw std::runtime_error("failed to map vertex buffer");

    std::memcpy(mapped, triangleVerticies, sizeof(triangleVerticies));
    vertexBuffer->Unmap(0, nullptr);

    vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexView.StrideInBytes = sizeof(Vertex);
    vertexView.SizeInBytes = sizeof(triangleVerticies);
}

void Engine::createRootSignature() {
    D3D12_ROOT_SIGNATURE_DESC sigDesc{};
    sigDesc.NumParameters = 0;
    sigDesc.pParameters = nullptr;
    sigDesc.NumStaticSamplers = 0;
    sigDesc.pStaticSamplers = nullptr;
    sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sigBlob;
    ComPtr<ID3DBlob> errBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &sigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        sigBlob.GetAddressOf(),
        errBlob.GetAddressOf()
    );

    if (FAILED(hr)) {
        if (errBlob) {
            std::cerr << (const char*)errBlob->GetBufferPointer() << "\n";
        }
        throw std::runtime_error("D3D12SerializeRootSignature failed");
    }

    hr = device->CreateRootSignature(
        0, // nodeMask
        sigBlob->GetBufferPointer(),
        sigBlob->GetBufferSize(),
        IID_PPV_ARGS(rootSignature.GetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("CreateRootSignature failed");
    }
}

void Engine::createPipelineState() {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pso.SampleMask = UINT_MAX;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.SampleDesc.Count = 1;

    pso.pRootSignature = rootSignature.Get();

    pso.VS.pShaderBytecode  = g_const_color_vs;
    pso.VS.BytecodeLength = sizeof(g_const_color_vs);

    pso.PS.pShaderBytecode = g_const_color_ps;
    pso.PS.BytecodeLength = sizeof(g_const_color_ps);

    // Required defaults
    pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso.BlendState      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso.DepthStencilState.DepthEnable   = FALSE;
    pso.DepthStencilState.StencilEnable = FALSE;


    D3D12_INPUT_ELEMENT_DESC inputDesc{};
    inputDesc.SemanticName = "POSITION";
    inputDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
    inputDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

    pso.InputLayout = { &inputDesc, 1 };

    HRESULT hr = device->CreateGraphicsPipelineState(
        &pso,
        IID_PPV_ARGS(pipelineState.GetAddressOf())
    );
    if (FAILED(hr)) throw std::runtime_error("failed to crate graphics pipeline state");
}

void Engine::createVpAndSc() {
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width    = (float)width;
    vp.Height   = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    sc.left   = 0;
    sc.top    = 0;
    sc.right  = (LONG)width;
    sc.bottom = (LONG)height;
}


void Engine::renderFrame() {
    HRESULT hr;

    frameBegin();

    commandList[bi]->ResourceBarrier(1, &presentToRTVBarrier[bi]);

    commandList[bi]->OMSetRenderTargets(1, &rtvHandle[bi], FALSE, nullptr);

    commandList[bi]->ClearRenderTargetView(rtvHandle[bi], rendColor, 0, nullptr);

    commandList[bi]->SetPipelineState(pipelineState.Get());

    commandList[bi]->SetGraphicsRootSignature(rootSignature.Get());

    commandList[bi]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList[bi]->IASetVertexBuffers(0, 1, &vertexView);

    commandList[bi]->RSSetViewports(1, &vp);
    commandList[bi]->RSSetScissorRects(1, &sc);

    commandList[bi]->DrawInstanced(3, 1, 0, 0);

    commandList[bi]->ResourceBarrier(1, rtvToPresentBarrier + bi);

    hr = commandList[bi]->Close();
    if (FAILED(hr)) {
        throw std::runtime_error("failed to close command list");
    }

    ID3D12CommandList* lists[] = {commandList[bi].Get()};
    commandQueue->ExecuteCommandLists(_countof(lists), lists);

    const UINT64 signalValue = ++fenceValue[bi];
    hr = commandQueue->Signal(fence[bi].Get(), signalValue);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create command queue signal");
    }

    hr = swapChain->Present(1, 0);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to present buffer");
    }

    frameEnd();
}

void Engine::frameBegin() {
    HRESULT hr;

    bi = swapChain->GetCurrentBackBufferIndex();

    waitForGPURenderFrame();

    hr = commandAllocator[bi]->Reset();
    if (FAILED(hr)) {
        throw std::runtime_error("failed to reset command allocator");
    }

    hr = commandList[bi]->Reset(commandAllocator[bi].Get(), nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to reset command allocator");
    }

    float frameCoef = static_cast<float>(frameIdx % 1000) / 1000.f;

    rendColor[0] = frameCoef;
    rendColor[1] = 1.f - frameCoef;
    rendColor[2] = 0.f;
    rendColor[3] = 1.f;
}

void Engine::frameEnd() {
    frameIdx++;
}

void Engine::waitForGPURenderFrame() {
    if (fence[bi]->GetCompletedValue() < fenceValue[bi]) {
        HRESULT hr = fence[bi]->SetEventOnCompletion(fenceValue[bi], fenceEvent);
        if (FAILED(hr)) {
            throw std::runtime_error("failed to  set fence event on completion");

        }

        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void Engine::stopRendering() {
    // waitForGPURenderFrame();
}
