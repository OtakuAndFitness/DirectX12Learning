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

	void CreateDevice();
	void CreateFence();
	void GetDescriptorSize();
	void SetMSAA();
	void CreateCommandObject();
	void CreateSwapChain();
	//void CreateDescriptorHeap();
	void CreateViewPortAndScissorRect();

	void FlushCmdQueue();

	void CalculateFrameState();

	float AspectRatio()const;

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update() = 0;
	virtual void Draw() = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;


protected:
	static D3D12App* mApp;

	HINSTANCE mhAppInst = nullptr;
	HWND mhMainWnd = 0;

	GameTimer mTimer;

	static const int SwapChainBufferCount = 2;
	int mCurrentBackBuffer = 0;


	ComPtr<ID3D12Device> d3dDevice;
	ComPtr<IDXGIFactory4> dxgiFactory;
	ComPtr<ID3D12Fence> fence;
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

	UINT rtvDescriptorSize = 0;
	UINT dsvDescriptorSize = 0;
	UINT csuDescriptorSize = 0;

	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;

	int mCurrentFence = 0;//初始CPU上的围栏点为0

	int mClientWidth = 1280;
	int mClientHeight = 720;

	bool mAppPaused = false;  
	bool mMinimized = false; 
	bool mMaximized = false; 
	bool mResizing = false;

	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	wstring mMainWndCaption = L"d3d12 App";
};