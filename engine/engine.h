#ifndef ENGINE_H_
#define ENGINE_H_

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

    void frameBegin();
    void frameEnd();
    void waitForGPURenderFrame();

private:
    static const int bufferCount = 2;
    int bi{};

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

    float rendColor[4];
    UINT64 frameIdx = 0;

    HWND hwnd;
};

#endif
