#ifndef ENGINE_H_
#define ENGINE_H_

#include "types.h"

#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <QImage>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

using Microsoft::WRL::ComPtr;

class Engine {
public:
    Engine();
    Engine(HWND hwnd);

    ~Engine();

    void renderFrame();
    void stopRendering();

    int getFrameIdx();

private:
    void prepareForRendering();

    void createDevice();

    void createCommandsManagers();

    void createSwapChain();
    void createRenderTargetView();

    void createBarriers();
    void createFence();


    void createVertexBuffer();
    void createRootSignature();
    void createPipelineState();
    void createVpAndSc();

    void frameBegin();
    void frameEnd();
    void waitForGPURenderFrame();

    void generateHexagon(float x, float angle);

private:
    static const int bufferCount = 2;
    int bi{};

    UINT width = 600, height = 600;

    ComPtr<IDXGIFactory4> dxgiFactory{};
    ComPtr<ID3D12Device> device{};

    ComPtr<ID3D12CommandQueue> commandQueue{};
    ComPtr<ID3D12CommandAllocator> commandAllocator[bufferCount];
    ComPtr<ID3D12GraphicsCommandList1> commandList[bufferCount];

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    ComPtr<IDXGISwapChain3> swapChain{};

    ComPtr<ID3D12Resource> backBuffers[bufferCount];
    ComPtr<ID3D12DescriptorHeap> rtvHeap{};
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[bufferCount];

    D3D12_RESOURCE_BARRIER rtvToPresentBarrier[bufferCount];
    D3D12_RESOURCE_BARRIER presentToRTVBarrier[bufferCount];

    ComPtr<ID3D12Fence> fence[bufferCount];
    HANDLE fenceEvent;
    UINT64 fenceValue[bufferCount] = {0, 0};

    ComPtr<ID3D12Resource> vertexBuffer{};
    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    ComPtr<ID3D12RootSignature> rootSignature{};
    ComPtr<ID3D12PipelineState> pipelineState{};
    D3D12_VIEWPORT vp{};
    D3D12_RECT sc{};

    Vertex triangles[6][3];
    Vertex triangleVerticies[3] = {{0.0, 0.5}, {0.5, -0.5}, {-0.5, -0.5}};
    float rendColor[4] = {0.f, 0.5f, 0.f, 1.f};
    UINT64 frameIdx = 0;

    HWND hwnd;
};

#endif
