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

    void renderFrame();
    int getFrameIdx();
    QImage getQImageForFrame();

private:
    void prepareForRendering();

    void createDevice();

    void createCommandsManagers();

    void createGPUTexture();

    void createRenderTargetView();

    void createReadbackBuffer();
    void createCopyLocations();
    void createBarriers();

    void createFence();


    void frameBegin();
    void frameEnd();
    void waitForGPURenderFrame();

    void writeImageToQImage();
    void dumpRenderTargetToPPM();

private:
    ComPtr<IDXGIFactory4> dxgiFactory{};
    ComPtr<IDXGIAdapter1> adapter{};
    ComPtr<ID3D12Device> device{};

    ComPtr<ID3D12CommandQueue> commandQueue{};
    ComPtr<ID3D12CommandAllocator> commandAllocator{};
    ComPtr<ID3D12GraphicsCommandList1> commandList{};

    D3D12_RESOURCE_DESC textureDesc{};
    ComPtr<ID3D12Resource> renderTarget{};
    ComPtr<ID3D12DescriptorHeap> rtvHeap{};
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};

    ComPtr<ID3D12Resource> readbackBuffer{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT renderTargetFootprint{};

    D3D12_TEXTURE_COPY_LOCATION source{};
    D3D12_TEXTURE_COPY_LOCATION destination{};

    D3D12_RESOURCE_BARRIER rtvToCopyBarrier{};
    D3D12_RESOURCE_BARRIER copyToRTVBarrier{};

    ComPtr<ID3D12Fence> fence{};
    HANDLE fenceEvent{};
    UINT64 fenceValue = 1;

    float rendColor[4];
    UINT64 frameIdx = 0;

    QImage image;
};

#endif
