#include "D3D12App.h"

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam) {
	switch (msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lparam);
}

D3D12App::D3D12App()
{
}

D3D12App::~D3D12App()
{
}

bool D3D12App::Init(HINSTANCE hInstance, int nShowCmd) {
	if (!InitWindow(hInstance, nShowCmd)) {
		return false;
	}
	else if (!InitDirect3D()) {
		return false;
	}
	else {
		return true;
	}
}

bool D3D12App::InitWindow(HINSTANCE hInstance, int nShowCmd) {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;//水平和垂直方向改变是重绘窗口
	wc.lpfnWndProc = MainWndProc;//设置窗口过程为MainWndProc,用于处理窗口接收到的消息
	wc.cbClsExtra = 0;//指定窗口实例额外的内存空间，这里设置为0，即不分配额外空间
	wc.cbWndExtra = 0;//同上
	wc.hInstance = hInstance;//应用程序实例句柄
	wc.hIcon = LoadIcon(0, IDC_ARROW);//加载一个默认的图标作为窗口的图标
	wc.hCursor = LoadCursor(0, IDC_ARROW);//加载一个默认的光标作为窗口光标
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//窗口背景刷成黑色
	wc.lpszMenuName = 0;//没有菜单栏
	wc.lpszClassName = L"MainWindow";//窗口名

	if (!RegisterClass(&wc)) {
		//消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return 0;
	}


	RECT R;
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);//根据客户区矩形的大小和窗口样式计算窗口大小
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	//创建窗口
	mhMainWnd = CreateWindow(L"MainWindow", L"DXInitailize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInstance, 0);

	//窗口创建失败
	if (!mhMainWnd) {
		MessageBox(0, L"CreateWindow Failed", 0, 0);
		return 0;
	}

	//成功则显示并更新窗口
	ShowWindow(mhMainWnd, nShowCmd);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3D12App::InitDirect3D() {
	//1.开启D3D12调试层。
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif


	//2.创建设备。
	//3.创建围栏，同步CPU和GPU。
	//4.获取描述符大小。
	//5.设置MSAA抗锯齿属性。
	//6.创建命令队列、命令列表、命令分配器。
	//7.创建交换链。
	//8.创建描述符堆。
	//9.创建描述符。
	//10.资源转换。
	//11.设置视口和裁剪矩形。

	CreateDevice();
	CreateFence();
	GetDescriptorSize();
	SetMSAA();
	CreateCommandObject();
	CreateSwapChain();
	CreateDexcriptorHeap();
	CreateRTV();
	CreateDSV();
	CreateViewPortAndScissorRect();

	return true;
}

void D3D12App::CreateDevice() {
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3dDevice)));
}

void D3D12App::CreateFence() {
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void D3D12App::GetDescriptorSize() {
	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);//渲染目标缓冲区描述符
	dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);//深度模板缓冲区描述符
	csuDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);//常量缓冲区描述符、着色器资源缓冲描述符和随机访问缓冲描述符
}

void D3D12App::SetMSAA() {
	msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msaaQualityLevels.SampleCount = 1;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
	assert(msaaQualityLevels.NumQualityLevels > 0);
}

void D3D12App::CreateCommandObject() {
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
	cmdList->Close();
}

void D3D12App::CreateSwapChain() {
	swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width = 1280;
	swapChainDesc.BufferDesc.Height = 720;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = mhMainWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));
}

void D3D12App::CreateDexcriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvHeap)));
}
void D3D12App::CreateRTV() {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < 2; i++) {
		swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffer[i].ReleaseAndGetAddressOf()));
		d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
}

void D3D12App::CreateDSV() {
	D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsvResourceDesc.DepthOrArraySize = 1;
	dsvResourceDesc.Width = 1280;
	dsvResourceDesc.Height = 720;
	dsvResourceDesc.MipLevels = 1;
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvResourceDesc.SampleDesc.Count = 4;
	dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;
	CD3DX12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1;
	optClear.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &dsvResourceDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&depthStencilBuffer)));
	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}


void D3D12App::CreateViewPortAndScissorRect() {
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = 1280;
	viewPort.Height = 720;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	scissorRect.left = 0;
	scissorRect.right = 0;
	scissorRect.right = 1280;
	scissorRect.bottom = 720;
}

void D3D12App::FlushCmdQueue() {
	mCurrentFence++;
	cmdQueue->Signal(fence.Get(), mCurrentFence);
	if (fence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		fence->SetEventOnCompletion(mCurrentFence, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3D12App::CalculateFrameState() {
	static int frameCnt = 0;//总帧数
	static float timeElapsed = 0.0f;//流逝的时间
	frameCnt++;//每帧++，经过一秒后其即为FPS值

	if (gt.TotalTime() - timeElapsed >= 1.0f) {//一旦>=1，说明刚好过一秒
		float fps = (float)frameCnt;//每秒多少帧
		float mspf = 1000.0f / fps;//每帧多少毫秒

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = L"D3D12Init    fps:" + fpsStr + L"    " + L"mspf:" + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());
		//为计算下一组帧数值而重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

int D3D12App::Run() {
	//消息循环
	//定义消息结构体
	MSG msg = { 0 };
	//每次循环开始都要重置计时器
	gt.Reset();

	while (msg.message != WM_QUIT) {
		//如果有窗口消息就进行处理
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {//PeekMessage函数会自动填充msg结构体元素
			TranslateMessage(&msg);//键盘按键转换，将虚拟键消息转换为字符消息
			DispatchMessage(&msg);//把消息分派给相应的窗口过程

		}
		else {
			gt.Tick();//计算每两帧间隔时间
			if (!gt.IsStoped()) {//如果不是暂停状态，我们才运行游戏
				CalculateFrameState();
				//否则就执行动画和游戏逻辑
				Draw();
			}
			else {//如果是暂停状态，则休眠100秒
				Sleep(100);
			}

		}
	}

	return (int)msg.wParam;
}