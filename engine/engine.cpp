#include "engine.h"

#include <iostream>
#include <fstream>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

Engine::Engine() {
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

void Engine::renderFrame() {
    HRESULT hr;

    frameBegin();

    commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    commandList->ClearRenderTargetView(rtvHandle, rendColor, 0, nullptr);

    commandList->ResourceBarrier(1, &rtvToCopyBarrier);

    commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

    commandList->ResourceBarrier(1, &copyToRTVBarrier);

    hr = commandList->Close();
    if (FAILED(hr)) {
        throw std::runtime_error("failed to close command list");
    }

    ID3D12CommandList* lists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(_countof(lists), lists);

    hr = commandQueue->Signal(fence.Get(), fenceValue);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create command queue signal");
    }

    waitForGPURenderFrame();
    frameEnd();
}

int Engine::getFrameIdx() {
    return frameIdx;
}

QImage Engine::getQImageForFrame() {
    return image;
}

void Engine::prepareForRendering() {
    createDevice();

    createCommandsManagers();
    createGPUTexture();
    createRenderTargetView();

    createReadbackBuffer();
    createCopyLocations();
    createBarriers();

    createFence();
}

void Engine::createDevice() {
    HRESULT hr;

    hr = CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create device factory");
    }

    for (UINT i = 0; dxgiFactory->EnumAdapters1(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }
        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
            continue;
        }

        hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.GetAddressOf()));
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

    hr = device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(commandAllocator.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to crerate command allocator");
    }

    hr = device->CreateCommandList(0, queueDesc.Type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to crerate command list");
    }
}

void Engine::createGPUTexture() {
    HRESULT hr;

    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = 3;
    textureDesc.Height = 2;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        nullptr,
        IID_PPV_ARGS(renderTarget.GetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create texture");
    }
}

void Engine::createRenderTargetView() {
    HRESULT hr;

    D3D12_DESCRIPTOR_HEAP_DESC descrHeapDesc{};
    descrHeapDesc.NumDescriptors = 1;
    descrHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    hr = device->CreateDescriptorHeap(&descrHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create descriptor heap");
    }

    rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(
        renderTarget.Get(),
        nullptr,
        rtvHandle
    );
}


void Engine::createReadbackBuffer() {
    HRESULT hr;

    UINT64 totalBytes;
    device->GetCopyableFootprints(
        &textureDesc,
        0,
        1,
        0,
        &renderTargetFootprint,
        nullptr,
        nullptr,
        &totalBytes
    );

    D3D12_RESOURCE_DESC readbackBuffDesc{};
    readbackBuffDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    readbackBuffDesc.Width = totalBytes;
    readbackBuffDesc.Height = 1;
    readbackBuffDesc.DepthOrArraySize   = 1;
    readbackBuffDesc.MipLevels          = 1;
    readbackBuffDesc.Format = DXGI_FORMAT_UNKNOWN;
    readbackBuffDesc.SampleDesc.Count   = 1;
    readbackBuffDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;

    hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &readbackBuffDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(readbackBuffer.GetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("failed to create readback buffer");
    }
}

void Engine::createCopyLocations() {
    source.pResource = renderTarget.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    source.SubresourceIndex = 0;

    destination.pResource = readbackBuffer.Get();
    destination.PlacedFootprint = renderTargetFootprint;
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
}

void Engine::createBarriers() {
    rtvToCopyBarrier.Transition.pResource = renderTarget.Get();
    rtvToCopyBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    rtvToCopyBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

    copyToRTVBarrier.Transition.pResource = renderTarget.Get();
    copyToRTVBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    copyToRTVBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void Engine::createFence() {
    HRESULT hr;

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("failed tocreate fence");
    }

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr) {
        throw std::runtime_error("failed to create fence event");
    }
}

void Engine::frameBegin() {
    float frameCoef = static_cast<float>(frameIdx % 1000) / 1000.f;

    rendColor[0] = frameCoef;
    rendColor[1] = 1.f - frameCoef;
    rendColor[2] = 0.f;
    rendColor[3] = 1.f;
}

void Engine::frameEnd() {
    HRESULT hr;

    writeImageToQImage();

    hr = commandAllocator->Reset();
    if (FAILED(hr)) {
        throw std::runtime_error("failed to reset command allocator");
    }

    hr = commandList->Reset(commandAllocator.Get(), nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to reset command allocator");
    }

    frameIdx++;
}

void Engine::waitForGPURenderFrame() {
    if (fence->GetCompletedValue() < fenceValue) {
        HRESULT hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
        if (FAILED(hr)) {
            throw std::runtime_error("failed to set fence event on completion");
        }

        WaitForSingleObject(fenceEvent, INFINITE);
    }
    fenceValue++;
}

void Engine::writeImageToQImage() {
    HRESULT hr;

    void* pData;
    hr = readbackBuffer->Map(0, nullptr, &pData);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to map readback buffer");
    }

    const UINT width = static_cast<UINT>(textureDesc.Width);
    const UINT height = textureDesc.Height;
    const UINT rowPitch = renderTargetFootprint.Footprint.RowPitch;

    auto* srcBytes = static_cast<const uint8_t*>(pData);

    QImage frame(width, height, QImage::Format_RGBA8888);

    const UINT stride = frame.bytesPerLine();
    auto* dstBytes = static_cast<uint8_t*>(frame.bits());

    for (UINT y = 0; y < height; ++y)
    {
        const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(srcBytes + y * rowPitch);
        uint32_t* dstRow = reinterpret_cast<uint32_t*>(dstBytes + y * stride);

        for (UINT x = 0; x < width; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }

    image = frame;
}

void Engine::dumpRenderTargetToPPM() {
    HRESULT hr;

    void* pData;
    hr = readbackBuffer->Map(0, nullptr, &pData);
    if (FAILED(hr)) {
        throw std::runtime_error("failed to map readback buffer");
    }

    const UINT width = static_cast<UINT>(textureDesc.Width);
    const UINT height = textureDesc.Height;
    const UINT rowPitch = renderTargetFootprint.Footprint.RowPitch;
    const UINT bytesPerPixel = 4;

    auto* srcBytes = static_cast<const uint8_t*>(pData);
    std::cout << uint32_t(srcBytes[0]) << std::endl;
    std::cout << uint32_t(srcBytes[1]) << std::endl;
    std::cout << uint32_t(srcBytes[2]) << std::endl;
    std::cout << uint32_t(srcBytes[3]) << std::endl;

     // 2. Open a binary PPM (P6) file
    std::ofstream file("output.ppm", std::ios::binary);
    if (!file) {
        readbackBuffer->Unmap(0, nullptr);
        throw std::runtime_error("failed to open output file");
    }

    // PPM P6 header: "P6\n<width> <height>\n255\n"
    file << "P3\n" << width << " " << height << "\n255\n";

    // 3. Write pixels row by row
    // DX12 textures are top-left origin; PPM is also typically top-left,
    // so we can go y = 0..height-1 directly.
    for (UINT y = 0; y < height; ++y)
    {
        const uint8_t* rowStart = srcBytes + y * rowPitch;

        for (UINT x = 0; x < width; ++x)
        {
            const uint8_t* pixel = rowStart + x * bytesPerPixel;
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];
            // pixel[3] is alpha, ignored for PPM
            file << static_cast<unsigned int>(r) << " "
                << static_cast<unsigned int>(g) << " "
                << static_cast<unsigned int>(b) << " ";
        }
        file << "\n";
    }

    file.close();

    // 4. Unmap when done
    readbackBuffer->Unmap(0, nullptr);
}

void Engine::stopRendering() {
    // waitForGPURenderFrame();
}
