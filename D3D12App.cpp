#include "D3D12App.h"
#include <cassert>

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam) {
	/*switch (msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lparam);*/
	return D3D12App::GetApp()->MsgProc(hwnd, msg, wParam, lparam);
}

LRESULT D3D12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	//消息处理
	switch (msg)
	{
		case WM_SIZE:
			mClientWidth = LOWORD(lParam);
			mClientHeight = HIWORD(lParam);
			if (d3dDevice) {
				if (wParam == SIZE_MINIMIZED) {
					mAppPaused = true;
					mMinimized = true;
					mMaximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED) {
					mAppPaused = false;
					mMinimized = false;
					mMaximized = true;
					OnResize();
				}
				else if (wParam == SIZE_RESTORED) {
					if (mMinimized) {
						mAppPaused = false;
						mMinimized = false;
						OnResize();
					}
					else if (mMaximized) {
						mAppPaused = false;
						mMaximized = false;
						OnResize();
					}
					else if (mResizing) {

					}
					else {
						OnResize();
					}
				}
			}
			return 0;
			//鼠标按键按下时的触发（左中右）
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
				//wParam为输入的虚拟键代码，lParam为系统反馈的光标信息
				OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;
			//鼠标按键抬起时的触发（左中右）
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;
			//鼠标移动的触发
			case WM_MOUSEMOVE:
				OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;
			//当窗口被销毁时，终止消息循环
			case WM_DESTROY:
				PostQuitMessage(0);//终止消息循环，并发出WM_QUIT消息
				return 0;
			default:
				break;
	}
	//将上面没有处理的消息转发给默认的窗口过程
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

D3D12App* D3D12App::mApp = nullptr;
D3D12App* D3D12App::GetApp() {
	return mApp;
}
D3D12App::D3D12App(HINSTANCE hInstance) : mhAppInst(hInstance)
{
	assert(mApp == nullptr);
	mApp = this;
}

D3D12App::~D3D12App()
{
	if (d3dDevice != nullptr) {
		FlushCmdQueue();
	}
}

bool D3D12App::Init() {
	if (!InitWindow()) {
		return false;
	}
	
	if (!InitDirect3D()) {
		return false;
	}
	
	OnResize();
	
	return true;
	
}

bool D3D12App::InitWindow() {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;//水平和垂直方向改变是重绘窗口
	wc.lpfnWndProc = MainWndProc;//设置窗口过程为MainWndProc,用于处理窗口接收到的消息
	wc.cbClsExtra = 0;//指定窗口实例额外的内存空间，这里设置为0，即不分配额外空间
	wc.cbWndExtra = 0;//同上
	wc.hInstance = mhAppInst;//应用程序实例句柄
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
	R.right = mClientWidth;
	R.bottom = mClientHeight;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);//根据客户区矩形的大小和窗口样式计算窗口大小
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	//创建窗口
	mhMainWnd = CreateWindow(L"MainWindow", mMainWndCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

	//窗口创建失败
	if (!mhMainWnd) {
		MessageBox(0, L"CreateWindow Failed", 0, 0);
		return 0;
	}

	//成功则显示并更新窗口
	ShowWindow(mhMainWnd, SW_SHOW);
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
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3D12App::CreateDevice() {
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	ThrowIfFailed(D3D12CreateDevice(nullptr, //此参数如果设置为nullptr，则使用主适配器
		D3D_FEATURE_LEVEL_12_0, //应用程序需要硬件所支持的最低功能级别
		IID_PPV_ARGS(&d3dDevice)));//返回所建设备
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
	msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//UNORM是归一化处理的无符号整数
	msaaQualityLevels.SampleCount = 1;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;
	//当前图形驱动对MSAA多重采样的支持（注意：第二个参数即是输入又是输出）
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
	//NumQualityLevels在Check函数里会进行设置
	//如果支持MSAA，则Check函数返回的NumQualityLevels > 0
	//expression为假（即为0），则终止程序运行，并打印一条出错信息
	assert(msaaQualityLevels.NumQualityLevels > 0);
}

void D3D12App::CreateCommandObject() {
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(d3dDevice->CreateCommandList(0, //掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //命令列表类型
		cmdAllocator.Get(), //命令分配器接口指针
		nullptr, //流水线状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&cmdList)));//返回创建的命令列表
	cmdList->Close();//重置命令列表前必须将其关闭
}

void D3D12App::CreateSwapChain() {
	mSwapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc;//交换链描述结构体
	swapChainDesc.BufferDesc.Width = mClientWidth;//缓冲区分辨率的宽度
	swapChainDesc.BufferDesc.Height = mClientHeight;//缓冲区分辨率的高度
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//缓冲区的显示格式
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;//刷新率的分子
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;//刷新率的分母
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;//逐行扫描VS隔行扫描(未指定的)
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;//图像相对屏幕的拉伸（未指定的）
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//将数据渲染至后台缓冲区（即作为渲染目标）
	swapChainDesc.OutputWindow = mhMainWnd;//渲染窗口句柄
	swapChainDesc.SampleDesc.Count = 1;//多重采样数量
	swapChainDesc.SampleDesc.Quality = 0;//多重采样质量
	swapChainDesc.Windowed = true;//是否窗口化
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//固定写法
	swapChainDesc.BufferCount = 2;//后台缓冲区数量（双缓冲）
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//自适应窗口模式（自动选择最适于当前窗口尺寸的显示模式）
	//利用DXGI接口下的工厂类创建交换链
	ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, mSwapChain.GetAddressOf()));
}

void D3D12App::CreateViewPortAndScissorRect() {
	//视口设置
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = static_cast<float>(mClientWidth);
	viewPort.Height = static_cast<float>(mClientHeight);
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;
	//裁剪矩形设置（矩形外的像素都将被剔除）
	//前两个为左上点坐标，后两个为右下点坐标
	scissorRect.left = 0;
	scissorRect.right = 0;
	scissorRect.right = mClientWidth;
	scissorRect.bottom = mClientHeight;
}

void D3D12App::FlushCmdQueue() {
	mCurrentFence++;//CPU传完命令并关闭后，将当前围栏值+1
	cmdQueue->Signal(fence.Get(), mCurrentFence);//当GPU处理完CPU传入的命令后，将fence接口中的围栏值+1，即fence->GetCompletedValue()+1
	if (fence->GetCompletedValue() < mCurrentFence) {//如果小于，说明GPU没有处理完所有命令
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");//创建事件
		fence->SetEventOnCompletion(mCurrentFence, eventHandle);//当围栏达到mCurrentFence值（即执行到Signal（）指令修改了围栏值）时触发的eventHandle事件
		WaitForSingleObject(eventHandle, INFINITE);//等待GPU命中围栏，激发事件（阻塞当前线程直到事件触发，注意此Event需先设置再等待，
																				//如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
		CloseHandle(eventHandle);
	}
}

void D3D12App::CalculateFrameState() {
	static int frameCnt = 0;//总帧数
	static float timeElapsed = 0.0f;//流逝的时间
	frameCnt++;//每帧++，经过一秒后其即为FPS值

	if (mTimer.TotalTime() - timeElapsed >= 1.0f) {//一旦>=1，说明刚好过一秒
		float fps = (float)frameCnt;//每秒多少帧
		float mspf = 1000.0f / fps;//每帧多少毫秒

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption + L"    " + L"fps:" + fpsStr + L"    " + L"mspf:" + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());
		//为计算下一组帧数值而重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3D12App::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

}

void D3D12App::OnResize() {
	assert(d3dDevice);
	assert(mSwapChain);
	assert(cmdAllocator);

	FlushCmdQueue();

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
	
	for (int i = 0; i < SwapChainBufferCount; i++)
	{
		mSwapChainBuffer[i].Reset();
	}
	mDepthStencilBuffer.Reset();

	ThrowIfFailed(mSwapChain->ResizeBuffers(SwapChainBufferCount, mClientWidth, mClientHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	
	mCurrentBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < SwapChainBufferCount; i++) {
		//获得存于交换链中的后台缓冲区资源
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		//创建RTV
		d3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(),
			nullptr, //在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
			rtvHeapHandle);//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
		//偏移到描述符堆中的下一个缓冲区
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}

	//在CPU中创建好深度模板数据资源
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Alignment = 0;//指定对齐
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 指定资源维度（类型）为TEXTURE2D
	depthStencilDesc.DepthOrArraySize = 1;//纹理深度为1
	depthStencilDesc.Width = mClientWidth;//资源宽
	depthStencilDesc.Height = mClientHeight;//资源高
	depthStencilDesc.MipLevels = 1;//MIPMAP层级数量
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	depthStencilDesc.SampleDesc.Count = 1;//多重采样数量
	depthStencilDesc.SampleDesc.Quality = 0;//多重采样质量
	
	CD3DX12_CLEAR_VALUE optClear;//清除资源的优化值，提高清除操作的执行速度（CreateCommittedResource函数中传入）
	optClear.Format = mDepthStencilFormat;//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	optClear.DepthStencil.Depth = 1.0f;//初始深度值为1
	optClear.DepthStencil.Stencil = 0;//初始模板值为0
	//创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), //堆类型为默认堆（不能写入）
		D3D12_HEAP_FLAG_NONE, //Flag
		&depthStencilDesc, //上面定义的DSV资源指针
		D3D12_RESOURCE_STATE_COMMON, //资源的状态为初始状态
		&optClear, //上面定义的优化值指针
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));//返回深度模板资源

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	//创建DSV(必须填充DSV属性结构体，和创建RTV不同，RTV是通过句柄)
	d3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(),
		&dsvDesc, //D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
					//由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
		mDsvHeap->GetCPUDescriptorHandleForHeapStart());//DSV句柄
	
	ThrowIfFailed(cmdList->Close());//命令添加完后将其关闭
	ID3D12CommandList* cmdsLists[] = { cmdList.Get() };//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);//将命令从命令列表传至命令队列

	FlushCmdQueue();

	CreateViewPortAndScissorRect();

}

float D3D12App::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

int D3D12App::Run() {
	//消息循环
	//定义消息结构体
	MSG msg = { 0 };
	//每次循环开始都要重置计时器
	mTimer.Reset();

	while (msg.message != WM_QUIT) {
		//如果有窗口消息就进行处理
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {//PeekMessage函数会自动填充msg结构体元素
			TranslateMessage(&msg);//键盘按键转换，将虚拟键消息转换为字符消息
			DispatchMessage(&msg);//把消息分派给相应的窗口过程

		}
		else {
			mTimer.Tick();//计算每两帧间隔时间
			if (!mTimer.IsStoped()) {//如果不是暂停状态，我们才运行游戏
				CalculateFrameState();
				//否则就执行动画和游戏逻辑
				Update();
				Draw();
			}
			else {//如果是暂停状态，则休眠100秒
				Sleep(100);
			}

		}
	}

	return (int)msg.wParam;
}