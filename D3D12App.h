#pragma once

#include "DxException.h"
#include "GameTimer.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
//#pragma comment(lib, "dxguid.lib")



class D3D12App {
protected:
	D3D12App(HINSTANCE hInstance);
	D3D12App(const D3D12App& rhs) = delete;
	D3D12App& operator=(const D3D12App& rhs) = delete;
	virtual ~D3D12App();

public:
	static D3D12App* GetApp();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int Run();
	virtual bool Init();
	bool InitWindow();
	bool InitDirect3D();
	virtual void Update() = 0;
	virtual void Draw() = 0;

	void CreateDevice();
	void CreateFence();
	void GetDescriptorSize();
	void SetMSAA();
	void CreateCommandObject();
	void CreateSwapChain();
	void CreateDescriptorHeap();
	void CreateRTV();
	void CreateDSV();
	void CreateViewPortAndScissorRect();

	void FlushCmdQueue();

	void CalculateFrameState();

	virtual void OnResize();

protected:
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	static D3D12App* mApp;

	HINSTANCE mhAppInst = nullptr;
	HWND mhMainWnd = 0;

	GameTimer mTimer;

	ComPtr<ID3D12Device> d3dDevice;
	ComPtr<IDXGIFactory4> dxgiFactory;
	ComPtr<ID3D12Fence> fence;
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<IDXGISwapChain> swapChain;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12Resource> depthStencilBuffer;
	ComPtr<ID3D12Resource> swapChainBuffer[2];
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

	UINT rtvDescriptorSize = 0;
	UINT dsvDescriptorSize = 0;
	UINT csuDescriptorSize = 0;
	UINT mCurrentBackBuffer = 0;

	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;

	int mCurrentFence = 0;//初始CPU上的围栏点为0

	int mClientWidth = 1280;
	int mClientHeight = 720;

	bool mAppPaused = false;  
	bool mMinimized = false; 
	bool mMaximized = false; 
	bool mResizing = false;
};