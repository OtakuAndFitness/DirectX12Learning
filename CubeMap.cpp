//#include "D3D12App.h"
//#include "FrameResource.h"
//#include "Camera.h"
//#include "CubeRenderTarget.h"
//
//const UINT CubeMapSize = 512;
//
//struct RenderItem {
//	RenderItem() = default;
//
//	MeshGeometry* geo = nullptr;
//	Material* mat = nullptr;
//
//	//该几何体的世界矩阵
//	XMFLOAT4X4 world = MathHelper::Identity4x4();
//	//该几何体的顶点UV缩放矩阵
//	XMFLOAT4X4 texTransform = MathHelper::Identity4x4();
//
//	int NumFramesDirty = frameResourceCount;
//
//	//该几何体的常量数据在objConstantBuffer中的索引
//	UINT objCBIndex = -1;
//
//	//该几何体的图元拓扑类型
//	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//	//该几何体的绘制三参数
//	UINT indexCount = 0;
//	UINT startIndexLocation = 0;
//	UINT baseVertexLocation = 0;
//
//};
//
//enum class RenderLayer : int {
//	Opaque = 0,
//	OpaqueDynamicReflectors,
//	Sky,
//	Count
//};
//
//class CubeMap : public D3D12App {
//public:
//	CubeMap(HINSTANCE hInstance);
//	CubeMap(const CubeMap& rhs) = delete;
//	CubeMap& operator=(const CubeMap& rhs) = delete;
//	~CubeMap();
//
//	virtual bool Init()override;
//
//private:
//	virtual void OnResize()override;
//	virtual void Update()override;
//	virtual void Draw()override;
//	virtual void CreateRtvAndDsvDescriptorHeaps()override;
//
//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//
//	void OnKeyboardInput();
//	void UpdateObjectCBs();
//	void UpdateMaterialBuffer();
//	void UpdateMainPassCB();
//	void UpdateCubeMapFacePassCBs();
//
//	void LoadTextures();
//	void BuildRootSignature();
//	void BuildDescriptorHeaps();
//	void BuildCubeDepthStencil();
//	void BuildShadersAndInputLayout();
//	void BuildGeometry();
//	void BuildSkull();
//	void BuildPSO();
//	void BuildFrameResource();
//	void BuildMaterials();
//	void BuildRenderItem();
//	void DrawRenderItems(vector<RenderItem*>& ritems);
//	void DrawSceneToCubeMap();
//
//	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
//
//	void BuildCubeFaceCamera(float x, float y, float z);
//
//
//private:
//	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
//	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//
//	POINT mLastMousePos;
//	PassConstants passConstants;
//
//	vector<unique_ptr<RenderItem>> mAllRenderItems;
//
//	vector<unique_ptr<FrameResource>> mFrameResource;
//
//	FrameResource* mCurrFrameResource = nullptr;
//	int mCurrFrameResourceIndex = 0;
//	//UINT64 mCurrentFence = 0;//because of this, OnResize() function called then the window freezes
//
//	Camera mCamera;
//
//	UINT mSkyTexHeapIndex = 0;
//	UINT mDynamicTexHeapIndex = 0;
//
//	ComPtr<ID3D12Resource> mCubeDepthStencilBuffer;
//	CD3DX12_CPU_DESCRIPTOR_HANDLE mCubeDSV;
//
//	unordered_map<string, unique_ptr<Material>> mMaterials;
//	unordered_map<string, unique_ptr<Texture>> mTextures;
//	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
//	unordered_map<string, ComPtr<ID3DBlob>> mShaders;
//	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;
//
//	vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
//
//	RenderItem* mSkull = nullptr;
//
//	unique_ptr<CubeRenderTarget> mDynamicCubeMap = nullptr;
//	Camera mCubeMapCamera[6];
//};
//
//array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CubeMap::GetStaticSamplers()
//{
//	//过滤器POINT,寻址模式WRAP的静态采样器
//	CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,//着色器寄存器
//		D3D12_FILTER_MIN_MAG_MIP_POINT,//过滤器类型为POINT(常量插值)
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,//U方向上的寻址模式为WRAP（重复寻址模式）
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,//V方向上的寻址模式为WRAP（重复寻址模式）
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP);//W方向上的寻址模式为WRAP（重复寻址模式）
//
//	CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,
//		D3D12_FILTER_MIN_MAG_MIP_POINT,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
//
//	CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,
//		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP);
//
//	CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,
//		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
//
//	CD3DX12_STATIC_SAMPLER_DESC anisotropicWarp(4,
//		D3D12_FILTER_ANISOTROPIC,
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP);
//
//	CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,
//		D3D12_FILTER_ANISOTROPIC,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
//
//	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWarp, anisotropicClamp };
//}
//
//void CubeMap::BuildCubeFaceCamera(float x, float y, float z)
//{
//	XMFLOAT3 center(x, y, z);
//	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);
//
//	XMFLOAT3 targets[6] = {
//		XMFLOAT3(x + 1.0f, y, z),
//		XMFLOAT3(x - 1.0f, y, z),
//		XMFLOAT3(x, y + 1.0f, z),
//		XMFLOAT3(x, y - 1.0f, z),
//		XMFLOAT3(x, y, z + 1.0f),
//		XMFLOAT3(x, y, z - 1.0f),
//	};
//
//	XMFLOAT3 ups[6] = {
//		XMFLOAT3(0.0f,1.0f,0.0f),
//		XMFLOAT3(0.0f,1.0f,0.0f),
//		XMFLOAT3(0.0f,0.0f,-1.0f),
//		XMFLOAT3(0.0f,0.0f,1.0f),
//		XMFLOAT3(0.0f,1.0f,0.0f),
//		XMFLOAT3(0.0f,1.0f,0.0f),
//	};
//
//	for (int i = 0; i < 6; i++)
//	{
//		mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
//		mCubeMapCamera[i].SetLens(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
//		mCubeMapCamera[i].UpdateViewMatrix();
//	}
//}
//
//CubeMap::CubeMap(HINSTANCE hInstance) : D3D12App(hInstance)
//{
//}
//
//CubeMap::~CubeMap()
//{
//	if (d3dDevice != nullptr) {
//		FlushCmdQueue();
//	}
//}
//
//bool CubeMap::Init()
//{
//	if (!D3D12App::Init()) {
//		return false;
//	}
//
//	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
//
//	mDynamicCubeMap = make_unique<CubeRenderTarget>(d3dDevice.Get(), CubeMapSize, CubeMapSize, DXGI_FORMAT_R8G8B8A8_UNORM);
//
//	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
//
//	BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);
//
//	LoadTextures();
//	BuildRootSignature();
//	BuildDescriptorHeaps();
//	BuildCubeDepthStencil();
//	BuildShadersAndInputLayout();
//	BuildGeometry();
//	BuildSkull();
//	BuildMaterials();
//	BuildRenderItem();
//	BuildFrameResource();
//	BuildPSO();
//
//	ThrowIfFailed(cmdList->Close());
//	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
//	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
//
//	FlushCmdQueue();
//
//	return true;
//}
//
//void CubeMap::OnResize()
//{
//	D3D12App::OnResize();
//	
//	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//}
//
//void CubeMap::Update()
//{
//	OnKeyboardInput();
//
//	XMMATRIX skullScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
//	XMMATRIX skullOffset = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
//	XMMATRIX skullLocalRotate = XMMatrixRotationY(2.0f * mTimer.TotalTime());
//	XMMATRIX skullGlobalRotate = XMMatrixRotationY(0.5f * mTimer.TotalTime());
//	XMStoreFloat4x4(&mSkull->world, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);
//	mSkull->NumFramesDirty = frameResourceCount;
//
//	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % frameResourceCount;
//	mCurrFrameResource = mFrameResource[mCurrFrameResourceIndex].get();
//	//如果GPU端围栏值小于CPU端围栏值，即CPU速度快于GPU，则令CPU等待
//	if (mCurrFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < mCurrFrameResource->fenceCPU) {
//		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
//		ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->fenceCPU, eventHandle));
//		WaitForSingleObject(eventHandle, INFINITE);
//		CloseHandle(eventHandle);
//	}
//
//	UpdateObjectCBs();
//	UpdateMaterialBuffer();
//	UpdateMainPassCB();
//}
//
//void CubeMap::Draw()
//{
//	auto currCmdAllocator = mCurrFrameResource->cmdAllocator;
//	ThrowIfFailed(currCmdAllocator->Reset());//重复使用记录命令的相关内存
//	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSOs["opaque"].Get()));//复用命令列表及其内存
//
//	////设置CBV描述符堆
//	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
//	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//	////设置根签名
//	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
//
//	auto matBuffer = mCurrFrameResource->materialBuffer->Resource();
//	cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
//
//	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
//	skyTexDescriptor.Offset(mSkyTexHeapIndex, csuDescriptorSize);
//	cmdList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);
//
//	cmdList->SetGraphicsRootDescriptorTable(4, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
//
//	DrawSceneToCubeMap();
//
//	//设置视口和剪裁矩形
//	cmdList->RSSetViewports(1, &viewPort);
//	cmdList->RSSetScissorRects(1, &scissorRect);
//
//	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
//	//转换资源为后台缓冲区资源，从呈现到渲染目标转换
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
//	cmdList->ClearRenderTargetView(rtvHandle, Colors::LightBlue, 0, nullptr); //清除RT背景色为淡蓝，并且不设置裁剪矩形
//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
//	cmdList->ClearDepthStencilView(dsvHandle, //DSV描述符句柄
//		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, //Flag
//		1.0f, //默认深度值
//		0, //默认模板值
//		0, //裁剪矩形数量
//		nullptr);//裁剪矩形指针
//
//	cmdList->OMSetRenderTargets(1, //待绑定的RTV数量
//		&rtvHandle, //指向RTV数组的指针
//		true, //RTV对象在堆内存中是连续存放的
//		&dsvHandle);//指向DSV的指针
//
//	auto passCB = mCurrFrameResource->passCB->Resource();
//	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
//
//	CD3DX12_GPU_DESCRIPTOR_HANDLE dynamicTexDescriptor(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
//	dynamicTexDescriptor.Offset(mSkyTexHeapIndex + 1, csuDescriptorSize);
//	cmdList->SetGraphicsRootDescriptorTable(3, dynamicTexDescriptor);
//
//	DrawRenderItems(mRitemLayer[(int)RenderLayer::OpaqueDynamicReflectors]);
//
//	cmdList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);
//
//	//绘制顶点（通过索引缓冲区绘制）
//	DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);
//
//	cmdList->SetPipelineState(mPSOs["sky"].Get());
//	DrawRenderItems(mRitemLayer[(int)RenderLayer::Sky]);
//
//	//从渲染目标到呈现
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//	//完成命令的记录关闭命令列表
//	ThrowIfFailed(cmdList->Close());
//
//	ID3D12CommandList* cmdLists[] = { cmdList.Get() };//声明并定义命令列表数组
//	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);//将命令从命令列表传至命令队列
//
//	ThrowIfFailed(mSwapChain->Present(0, 0));
//	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
//
//	//FlushCmdQueue();
//	mCurrFrameResource->fenceCPU = ++mCurrentFence;
//	cmdQueue->Signal(fence.Get(), mCurrentFence);
//}
//
//void CubeMap::CreateRtvAndDsvDescriptorHeaps()
//{
//	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
//	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 6;
//	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	rtvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));
//
//	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
//	dsvHeapDesc.NumDescriptors = 2;
//	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
//	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	dsvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
//
//	mCubeDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1, dsvDescriptorSize);
//}
//
//void CubeMap::BuildDescriptorHeaps()
//{
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 5;
//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	//srvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc,
//		IID_PPV_ARGS(&mSrvHeap)));
//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
//
//	auto bricksTex = mTextures["bricksTex"]->resource;
//	auto tileTex = mTextures["tileTex"]->resource;
//	auto defaultTex = mTextures["defaultTex"]->resource;
//	auto skyCubeTex = mTextures["skyCubeTex"]->resource;
//
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//	srvDesc.Format = bricksTex->GetDesc().Format;
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
//	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
//	d3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = tileTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = defaultTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = defaultTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(defaultTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
//	srvDesc.TextureCube.MostDetailedMip = 0;
//	srvDesc.TextureCube.MipLevels = skyCubeTex->GetDesc().MipLevels;
//	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
//	srvDesc.Format = skyCubeTex->GetDesc().Format;
//	d3dDevice->CreateShaderResourceView(skyCubeTex.Get(), &srvDesc, handle);
//
//	mSkyTexHeapIndex = 3;
//	mDynamicTexHeapIndex = mSkyTexHeapIndex + 1;
//
//	auto srvCpuStart = mSrvHeap->GetCPUDescriptorHandleForHeapStart();
//	auto srvGpuStart = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
//	auto rtvCpuStart = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
//
//	int rtvOffset = SwapChainBufferCount;
//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE cubeRtvHandles[6];
//	for (int i = 0; i < 6; i++)
//	{
//		cubeRtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, rtvOffset + i, rtvDescriptorSize);
//	}
//
//	mDynamicCubeMap->BuildDescriptors(
//		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mDynamicTexHeapIndex, csuDescriptorSize), 
//		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mDynamicTexHeapIndex, csuDescriptorSize), 
//		cubeRtvHandles);
//}
//
//void CubeMap::BuildCubeDepthStencil()
//{
//	D3D12_RESOURCE_DESC depthStencilDesc;
//	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	depthStencilDesc.Alignment = 0;
//	depthStencilDesc.Width = CubeMapSize;
//	depthStencilDesc.Height = CubeMapSize;
//	depthStencilDesc.DepthOrArraySize = 1;
//	depthStencilDesc.MipLevels = 1;
//	depthStencilDesc.Format = mDepthStencilFormat;
//	depthStencilDesc.SampleDesc.Count = 1;
//	depthStencilDesc.SampleDesc.Quality = 0;
//	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	D3D12_CLEAR_VALUE optClear;
//	optClear.Format = mDepthStencilFormat;
//	optClear.DepthStencil.Depth = 1.0f;
//	optClear.DepthStencil.Stencil = 0;
//	ThrowIfFailed(d3dDevice->CreateCommittedResource(
//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
//		D3D12_HEAP_FLAG_NONE, 
//		&depthStencilDesc, 
//		D3D12_RESOURCE_STATE_COMMON, 
//		&optClear, 
//		IID_PPV_ARGS(mCubeDepthStencilBuffer.GetAddressOf())));
//
//	d3dDevice->CreateDepthStencilView(mCubeDepthStencilBuffer.Get(), nullptr, mCubeDSV);
//
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
//}
//
//void CubeMap::BuildShadersAndInputLayout()
//{
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["standardPS"] = d3dUtil::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_1");
//
//	mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
//
//	mInputLayout =
//	{
//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//	};
//}
//
//void CubeMap::BuildGeometry()
//{
//	ProceduralGeometry geoMetry;
//	ProceduralGeometry::MeshData box = geoMetry.CreateBox(1.0f, 1.0f, 1.0f, 3);
//	ProceduralGeometry::MeshData grid = geoMetry.CreateGrid(20.0f, 30.0f, 60, 40);
//	ProceduralGeometry::MeshData sphere = geoMetry.CreateSphere(0.5f, 20, 20);
//	ProceduralGeometry::MeshData cylinder = geoMetry.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
//
//	//计算单个几何体顶点在总顶点数组中的偏移量,顺序为：box、grid、sphere、cylinder
//	UINT boxVertexOffset = 0;
//	UINT gridVertexOffset = (UINT)box.Vertices.size();
//	UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
//	UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;
//
//	//计算单个几何体索引在总索引数组中的偏移量,顺序为：box、grid、sphere、cylinder
//	UINT boxIndexOffset = 0;
//	UINT gridIndexOffset = (UINT)box.Indices32.size();
//	UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
//	UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;
//
//	SubmeshGeometry boxSubMesh;
//	boxSubMesh.BaseVertexLocation = boxVertexOffset;
//	boxSubMesh.IndexCount = (UINT)box.Indices32.size();
//	boxSubMesh.StartIndexLocation = boxIndexOffset;
//
//	SubmeshGeometry gridSubMesh;
//	gridSubMesh.BaseVertexLocation = gridVertexOffset;
//	gridSubMesh.IndexCount = (UINT)grid.Indices32.size();
//	gridSubMesh.StartIndexLocation = gridIndexOffset;
//
//	SubmeshGeometry sphereSubMesh;
//	sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
//	sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
//	sphereSubMesh.StartIndexLocation = sphereIndexOffset;
//
//	SubmeshGeometry cylinderSubMesh;
//	cylinderSubMesh.BaseVertexLocation = cylinderVertexOffset;
//	cylinderSubMesh.IndexCount = (UINT)cylinder.Indices32.size();
//	cylinderSubMesh.StartIndexLocation = cylinderIndexOffset;
//
//	auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
//	vector<Vertex> vertices(totalVertexCount);//给定顶点数组大小
//
//	int k = 0;
//	for (int i = 0; i < box.Vertices.size(); i++, k++)
//	{
//		vertices[k].Pos = box.Vertices[i].Position;
//		vertices[k].Normal = box.Vertices[i].Normal;
//		vertices[k].TexC = box.Vertices[i].TexC;
//	}
//
//	for (int i = 0; i < grid.Vertices.size(); i++, k++)
//	{
//		vertices[k].Pos = grid.Vertices[i].Position;
//		vertices[k].Normal = grid.Vertices[i].Normal;
//		vertices[k].TexC = grid.Vertices[i].TexC;
//	}
//
//	for (int i = 0; i < sphere.Vertices.size(); i++, k++)
//	{
//		vertices[k].Pos = sphere.Vertices[i].Position;
//		vertices[k].Normal = sphere.Vertices[i].Normal;
//		vertices[k].TexC = sphere.Vertices[i].TexC;
//	}
//
//	for (int i = 0; i < cylinder.Vertices.size(); i++, k++)
//	{
//		vertices[k].Pos = cylinder.Vertices[i].Position;
//		vertices[k].Normal = cylinder.Vertices[i].Normal;
//		vertices[k].TexC = cylinder.Vertices[i].TexC;
//	}
//
//	vector<uint16_t> indices;
//	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
//	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
//	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
//	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());
//
//	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);
//
//	auto mGeo = make_unique<MeshGeometry>();
//	mGeo->Name = "shapeGeo";
//
//	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeo->VertexBufferCPU));
//	CopyMemory(mGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
//
//	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeo->IndexBufferCPU));
//	CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//	mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, mGeo->VertexBufferUploader);
//
//	mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, mGeo->IndexBufferUploader);
//
//	mGeo->VertexByteStride = sizeof(Vertex);
//	mGeo->VertexBufferByteSize = vbByteSize;
//	mGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
//	mGeo->IndexBufferByteSize = ibByteSize;
//
//	mGeo->DrawArgs["box"] = boxSubMesh;
//	mGeo->DrawArgs["grid"] = gridSubMesh;
//	mGeo->DrawArgs["sphere"] = sphereSubMesh;
//	mGeo->DrawArgs["cylinder"] = cylinderSubMesh;
//
//	mGeometries[mGeo->Name] = move(mGeo);
//}
//
//void CubeMap::BuildSkull()
//{
//	ifstream fin("Models/skull.txt");
//
//	if (!fin) {
//		MessageBox(0, L"Models/skull.txt not found", 0, 0);
//		return;
//	}
//
//	UINT vcount = 0;
//	UINT tcount = 0;
//
//	string ignore;
//
//	fin >> ignore >> vcount;
//	fin >> ignore >> tcount;
//	fin >> ignore >> ignore >> ignore >> ignore;
//
//	vector<Vertex> vertices(vcount);
//	for (UINT i = 0; i < vcount; i++)
//	{
//		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
//		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
//
//		vertices[i].TexC = { 0.0f, 0.0f };
//	}
//
//	fin >> ignore;
//	fin >> ignore;
//	fin >> ignore;
//
//	vector<int32_t> indices(3 * tcount);
//	for (UINT i = 0; i < tcount; i++)
//	{
//		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
//	}
//	
//	fin.close();
//
//	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//	const UINT ibByteSize = (UINT)indices.size() * sizeof(int32_t);
//
//	auto geo = make_unique<MeshGeometry>();
//	geo->Name = "skullGeo";
//
//	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));//创建顶点数据内存空间
//	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);//将顶点数据拷贝至顶点系统内存中
//
//	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));//创建索引数据内存空间
//	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);//将索引数据拷贝至索引系统内存中
//
//	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
//
//	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
//
//	geo->VertexByteStride = sizeof(Vertex);
//	geo->VertexBufferByteSize = vbByteSize;
//	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
//	geo->IndexBufferByteSize = ibByteSize;
//
//	SubmeshGeometry skullMesh;
//	skullMesh.IndexCount = (UINT)indices.size();
//	skullMesh.StartIndexLocation = 0;
//	skullMesh.BaseVertexLocation = 0;
//
//	geo->DrawArgs[geo->Name] = skullMesh;
//
//	mGeometries[geo->Name] = move(geo);
//}
//
//void CubeMap::BuildPSO()
//{
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
//	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
//	opaqueDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
//	opaqueDesc.pRootSignature = mRootSignature.Get();
//	opaqueDesc.VS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
//		mShaders["standardVS"]->GetBufferSize()
//	};
//	opaqueDesc.PS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["standardPS"]->GetBufferPointer()),
//		mShaders["standardPS"]->GetBufferSize()
//	};
//	opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	//opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
//	opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	opaqueDesc.SampleMask = UINT_MAX;
//	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	opaqueDesc.NumRenderTargets = 1;
//	opaqueDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//	opaqueDesc.SampleDesc.Count = 1;
//	opaqueDesc.SampleDesc.Quality = 0;
//	opaqueDesc.DSVFormat = mDepthStencilFormat;
//	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaqueDesc;
//	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//
//	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//	skyPsoDesc.pRootSignature = mRootSignature.Get();
//	skyPsoDesc.VS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
//		mShaders["skyVS"]->GetBufferSize()
//	};
//	skyPsoDesc.PS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
//		mShaders["skyPS"]->GetBufferSize()
//	};
//	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));
//
//}
//
//void CubeMap::BuildFrameResource()
//{
//	for (int i = 0; i < frameResourceCount; i++)
//	{
//		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
//			1+6, //passCount
//			(UINT)mAllRenderItems.size(),//objCount
//			(UINT)mMaterials.size()));
//	}
//}
//
//void CubeMap::BuildMaterials()
//{
//	auto bricks = make_unique<Material>();
//	bricks->name = "bricksMat";
//	bricks->matCBIndex = 0;
//	bricks->diffuseSrvHeapIndex = 0;
//	bricks->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	bricks->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	bricks->roughness = 0.3f;
//
//	auto tile = make_unique<Material>();
//	tile->name = "tileMat";
//	tile->matCBIndex = 1;
//	tile->diffuseSrvHeapIndex = 1;
//	tile->diffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
//	tile->fresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
//	tile->roughness = 0.1f;
//
//	auto mirror = make_unique<Material>();
//	mirror->name = "mirrorMat";
//	mirror->matCBIndex = 2;
//	mirror->diffuseSrvHeapIndex = 2;
//	mirror->diffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.1f, 1.0f);
//	mirror->fresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
//	mirror->roughness = 0.1f;
//
//	auto skull = make_unique<Material>();
//	skull->name = "skullMat";
//	skull->matCBIndex = 3;
//	skull->diffuseSrvHeapIndex = 2;
//	skull->diffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
//	skull->fresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
//	skull->roughness = 0.2f;
//
//	auto sky = make_unique<Material>();
//	sky->name = "skyMat";
//	sky->matCBIndex = 4;
//	sky->diffuseSrvHeapIndex = 3;
//	sky->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	sky->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	sky->roughness = 1.0f;
//
//	mMaterials[bricks->name] = move(bricks);
//	mMaterials[mirror->name] = move(mirror);
//	mMaterials[tile->name] = move(tile);
//	mMaterials[skull->name] = move(skull);
//	mMaterials[sky->name] = move(sky);
//}
//
//void CubeMap::BuildRenderItem()
//{
//	auto skyRenderItem = make_unique<RenderItem>();
//	XMStoreFloat4x4(&(skyRenderItem->world), XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
//	skyRenderItem->texTransform = MathHelper::Identity4x4();
//	skyRenderItem->objCBIndex = 0;
//	skyRenderItem->mat = mMaterials["skyMat"].get();
//	skyRenderItem->geo = mGeometries["shapeGeo"].get();
//	skyRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	skyRenderItem->indexCount = skyRenderItem->geo->DrawArgs["sphere"].IndexCount;
//	skyRenderItem->baseVertexLocation = skyRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
//	skyRenderItem->startIndexLocation = skyRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
//	mRitemLayer[(int)RenderLayer::Sky].push_back(skyRenderItem.get());
//	mAllRenderItems.push_back(move(skyRenderItem));
//
//	auto boxRenderItem = make_unique<RenderItem>();
//	XMStoreFloat4x4(&boxRenderItem->world, XMMatrixScaling(2.0f, 1.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
//	XMStoreFloat4x4(&boxRenderItem->texTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
//	boxRenderItem->objCBIndex = 1;
//	boxRenderItem->mat = mMaterials["bricksMat"].get();
//	boxRenderItem->geo = mGeometries["shapeGeo"].get();
//	boxRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	boxRenderItem->indexCount = boxRenderItem->geo->DrawArgs["box"].IndexCount;
//	boxRenderItem->baseVertexLocation = boxRenderItem->geo->DrawArgs["box"].BaseVertexLocation;
//	boxRenderItem->startIndexLocation = boxRenderItem->geo->DrawArgs["box"].StartIndexLocation;
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRenderItem.get());
//	mAllRenderItems.push_back(move(boxRenderItem));
//
//	auto skullRenderItem = make_unique<RenderItem>();
//	XMStoreFloat4x4(&skullRenderItem->world, XMMatrixScaling(0.4f, 0.4f, 0.4f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
//	skullRenderItem->texTransform = MathHelper::Identity4x4();
//	skullRenderItem->objCBIndex = 2;
//	skullRenderItem->mat = mMaterials["skullMat"].get();
//	skullRenderItem->geo = mGeometries["skullGeo"].get();
//	skullRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	skullRenderItem->indexCount = skullRenderItem->geo->DrawArgs["skullGeo"].IndexCount;
//	skullRenderItem->baseVertexLocation = skullRenderItem->geo->DrawArgs["skullGeo"].BaseVertexLocation;
//	skullRenderItem->startIndexLocation = skullRenderItem->geo->DrawArgs["skullGeo"].StartIndexLocation;
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());
//	mSkull = skullRenderItem.get();
//	mAllRenderItems.push_back(move(skullRenderItem));
//
//	auto gridRenderItem = make_unique<RenderItem>();
//	gridRenderItem->world = MathHelper::Identity4x4();
//	XMStoreFloat4x4(&gridRenderItem->texTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
//	gridRenderItem->objCBIndex = 3;
//	gridRenderItem->mat = mMaterials["tileMat"].get();
//	gridRenderItem->geo = mGeometries["shapeGeo"].get();
//	gridRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	gridRenderItem->indexCount = gridRenderItem->geo->DrawArgs["grid"].IndexCount;
//	gridRenderItem->baseVertexLocation = gridRenderItem->geo->DrawArgs["grid"].BaseVertexLocation;
//	gridRenderItem->startIndexLocation = gridRenderItem->geo->DrawArgs["grid"].StartIndexLocation;
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());
//	mAllRenderItems.push_back(move(gridRenderItem));
//
//	auto globeRenderItem = make_unique<RenderItem>();
//	XMStoreFloat4x4(&globeRenderItem->world, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 2.0f, 0.0f));
//	XMStoreFloat4x4(&globeRenderItem->texTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
//	globeRenderItem->objCBIndex = 4;
//	globeRenderItem->mat = mMaterials["mirrorMat"].get();
//	globeRenderItem->geo = mGeometries["shapeGeo"].get();
//	globeRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	globeRenderItem->indexCount = globeRenderItem->geo->DrawArgs["sphere"].IndexCount;
//	globeRenderItem->baseVertexLocation = globeRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
//	globeRenderItem->startIndexLocation = globeRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
//	mRitemLayer[(int)RenderLayer::OpaqueDynamicReflectors].push_back(globeRenderItem.get());
//	mAllRenderItems.push_back(move(globeRenderItem));
//
//	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
//	UINT followObjCBIndex = 5;//接下去的几何体常量数据在CB中的索引从2开始
//	//将圆柱和圆的实例模型存入渲染项中
//	for (int i = 0; i < 5; i++)
//	{
//		auto leftCylinderRenderItem = make_unique<RenderItem>();
//		auto rightCylinderRenderItem = make_unique<RenderItem>();
//		auto leftSphereRenderItem = make_unique<RenderItem>();
//		auto rightSphereRenderItem = make_unique<RenderItem>();
//
//		XMMATRIX leftCylinderWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
//		XMMATRIX rightCylinderWorld = XMMatrixTranslation(5.0f, 1.5f, -10.0f + i * 5.0f);
//		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
//		XMMATRIX rightSphereWorld = XMMatrixTranslation(5.0f, 3.5f, -10.0f + i * 5.0f);
//
//		//左边5个圆柱
//		XMStoreFloat4x4(&(leftCylinderRenderItem->world), leftCylinderWorld);
//		XMStoreFloat4x4(&(leftCylinderRenderItem->texTransform), brickTexTransform);
//		//此处的索引随着循环不断加1（注意：这里是先赋值再++）
//		leftCylinderRenderItem->objCBIndex = followObjCBIndex++;
//		leftCylinderRenderItem->mat = mMaterials["bricksMat"].get();
//		leftCylinderRenderItem->geo = mGeometries["shapeGeo"].get();
//		leftCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftCylinderRenderItem->indexCount = leftCylinderRenderItem->geo->DrawArgs["cylinder"].IndexCount;
//		leftCylinderRenderItem->baseVertexLocation = leftCylinderRenderItem->geo->DrawArgs["cylinder"].BaseVertexLocation;
//		leftCylinderRenderItem->startIndexLocation = leftCylinderRenderItem->geo->DrawArgs["cylinder"].StartIndexLocation;
//		//右边5个圆柱
//		XMStoreFloat4x4(&(rightCylinderRenderItem->world), rightCylinderWorld);
//		XMStoreFloat4x4(&(rightCylinderRenderItem->texTransform), brickTexTransform);
//		rightCylinderRenderItem->objCBIndex = followObjCBIndex++;
//		rightCylinderRenderItem->mat = mMaterials["bricksMat"].get();
//		rightCylinderRenderItem->geo = mGeometries["shapeGeo"].get();
//		rightCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightCylinderRenderItem->indexCount = rightCylinderRenderItem->geo->DrawArgs["cylinder"].IndexCount;
//		rightCylinderRenderItem->baseVertexLocation = rightCylinderRenderItem->geo->DrawArgs["cylinder"].BaseVertexLocation;
//		rightCylinderRenderItem->startIndexLocation = rightCylinderRenderItem->geo->DrawArgs["cylinder"].StartIndexLocation;
//		//左边5个球
//		XMStoreFloat4x4(&(leftSphereRenderItem->world), leftSphereWorld);
//		leftSphereRenderItem->texTransform = MathHelper::Identity4x4();
//		leftSphereRenderItem->objCBIndex = followObjCBIndex++;
//		leftSphereRenderItem->mat = mMaterials["mirrorMat"].get();
//		leftSphereRenderItem->geo = mGeometries["shapeGeo"].get();
//		leftSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftSphereRenderItem->indexCount = leftSphereRenderItem->geo->DrawArgs["sphere"].IndexCount;
//		leftSphereRenderItem->baseVertexLocation = leftSphereRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
//		leftSphereRenderItem->startIndexLocation = leftSphereRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
//		//右边5个球
//		XMStoreFloat4x4(&(rightSphereRenderItem->world), rightSphereWorld);
//		rightSphereRenderItem->texTransform = MathHelper::Identity4x4();
//		rightSphereRenderItem->mat = mMaterials["mirrorMat"].get();
//		rightSphereRenderItem->geo = mGeometries["shapeGeo"].get();
//		rightSphereRenderItem->objCBIndex = followObjCBIndex++;
//		rightSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightSphereRenderItem->indexCount = rightSphereRenderItem->geo->DrawArgs["sphere"].IndexCount;
//		rightSphereRenderItem->baseVertexLocation = rightSphereRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
//		rightSphereRenderItem->startIndexLocation = rightSphereRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
//
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylinderRenderItem.get());
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylinderRenderItem.get());
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRenderItem.get());
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRenderItem.get());
//
//		mAllRenderItems.push_back(move(leftCylinderRenderItem));
//		mAllRenderItems.push_back(move(rightCylinderRenderItem));
//		mAllRenderItems.push_back(move(leftSphereRenderItem));
//		mAllRenderItems.push_back(move(rightSphereRenderItem));
//
//	}
//}
//
//void CubeMap::DrawRenderItems(vector<RenderItem*>& ritems)
//{
//	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//	auto objectCB = mCurrFrameResource->objCB->Resource();
//
//	for (size_t i = 0; i < ritems.size(); i++)
//	{
//		auto renderItem = ritems[i];
//
//		cmdList->IASetVertexBuffers(0, 1, &renderItem->geo->GetVbv());
//		cmdList->IASetIndexBuffer(&renderItem->geo->GetIbv());
//		cmdList->IASetPrimitiveTopology(renderItem->primitiveType);
//
//		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + renderItem->objCBIndex * objCBByteSize;
//
//		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
//
//		cmdList->DrawIndexedInstanced(renderItem->indexCount, 1, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
//	}
//}
//
//void CubeMap::DrawSceneToCubeMap()
//{
//	cmdList->RSSetViewports(1, &mDynamicCubeMap->Viewport());
//	cmdList->RSSetScissorRects(1, &mDynamicCubeMap->ScissorRect());
//
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDynamicCubeMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
//
//	for (int i = 0; i < 6; i++)
//	{
//		cmdList->ClearRenderTargetView(mDynamicCubeMap->Rtv(i), Colors::LightBlue, 0, nullptr);
//		cmdList->ClearDepthStencilView(mCubeDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//		cmdList->OMSetRenderTargets(1, &mDynamicCubeMap->Rtv(i), true, &mCubeDSV);
//
//		auto passCB = mCurrFrameResource->passCB->Resource();
//		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (1 + i) * passCBByteSize;
//		cmdList->SetGraphicsRootConstantBufferView(1, passCBAddress);
//
//		DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);
//
//		cmdList->SetPipelineState(mPSOs["sky"].Get());
//		DrawRenderItems(mRitemLayer[(int)RenderLayer::Sky]);
//
//		cmdList->SetPipelineState(mPSOs["opaque"].Get());
//	}
//
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDynamicCubeMap->Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
//
//}
//
//void CubeMap::OnMouseDown(WPARAM btnState, int x, int y)
//{
//	mLastMousePos.x = x;
//	mLastMousePos.y = y;
//
//	SetCapture(mhMainWnd);
//}
//
//void CubeMap::OnMouseUp(WPARAM btnState, int x, int y)
//{
//	ReleaseCapture();
//}
//
//void CubeMap::OnMouseMove(WPARAM btnState, int x, int y)
//{
//	if ((btnState & MK_LBUTTON) != 0) {
//		//将鼠标的移动距离换算成弧度，0.25为调节阈值
//		float dx = XMConvertToRadians(static_cast<float>(x - mLastMousePos.x) * 0.25f);
//		float dy = XMConvertToRadians(static_cast<float>(y - mLastMousePos.y) * 0.25f);
//		//计算鼠标没有松开前的累计弧度
//		mCamera.Pitch(dy);
//		mCamera.Yaw(dx);
//	}
//
//	mLastMousePos.x = x;
//	mLastMousePos.y = y;
//}
//
//void CubeMap::OnKeyboardInput()
//{
//	const float dt = mTimer.DeltaTime();
//
//	if (GetAsyncKeyState('W') & 0x8000) {
//		mCamera.Walk(10.0f * dt);
//	}
//
//	if (GetAsyncKeyState('S') & 0x8000) {
//		mCamera.Walk(-10.0f * dt);
//	}
//
//	if (GetAsyncKeyState('A') & 0x8000) {
//		mCamera.Strafe(-10.0f * dt);
//	}
//
//	if (GetAsyncKeyState('D') & 0x8000) {
//		mCamera.Strafe(10.0f * dt);
//	}
//
//	mCamera.UpdateViewMatrix();
//}
//
//void CubeMap::UpdateObjectCBs()
//{
//	auto currObjectCB = mCurrFrameResource->objCB.get();
//
//	for (auto& e : mAllRenderItems)
//	{
//		if (e->NumFramesDirty > 0) {
//			XMMATRIX world = XMLoadFloat4x4(&e->world);
//			XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);
//
//			ObjectConstants objConstants;
//			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));
//			XMStoreFloat4x4(&objConstants.texTransform, XMMatrixTranspose(texTransform));
//			objConstants.materialIndex = e->mat->matCBIndex;
//			
//			currObjectCB->CopyData(e->objCBIndex, objConstants);
//
//			e->NumFramesDirty--;
//		}
//
//	}
//}
//
//void CubeMap::UpdateMaterialBuffer()
//{
//	auto currMaterialCB = mCurrFrameResource->materialBuffer.get();
//
//	for (auto& e : mMaterials)
//	{
//		Material* mat = e.second.get();//获得键值对的值，即Material指针（智能指针转普通指针）
//		if (mat->numFramesDirty > 0) {
//			//将定义的材质属性传给常量结构体中的元素
//
//			MaterialData matData;
//			matData.diffuseAlbedo = mat->diffuseAlbedo;
//			matData.fresnelR0 = mat->fresnelR0;
//			matData.roughness = mat->roughness;
//
//			XMMATRIX matTransform = XMLoadFloat4x4(&mat->matTransform);
//			XMStoreFloat4x4(&matData.matTransform, XMMatrixTranspose(matTransform));
//			matData.diffuseMapIndex = mat->diffuseSrvHeapIndex;
//
//			//将材质常量数据复制到常量缓冲区对应索引地址处
//			currMaterialCB->CopyData(mat->matCBIndex, matData);
//			//更新下一个帧资源
//			mat->numFramesDirty--;
//		}
//	}
//}
//
//void CubeMap::UpdateMainPassCB()
//{
//	XMMATRIX view = mCamera.GetView();
//	XMMATRIX proj = mCamera.GetProj();
//
//	XMMATRIX VP_Matrix = XMMatrixMultiply(view, proj);
//
//	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));
//
//	passConstants.eyePosW = mCamera.GetPosition3f();
//
//	passConstants.ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
//	passConstants.lights[0].direction = { 0.57735f, -0.57735f, 0.57735f };
//	passConstants.lights[0].strength = { 0.8f, 0.8f, 0.8f };
//	passConstants.lights[1].direction = { -0.57735f, -0.57735f, 0.57735f };
//	passConstants.lights[1].strength = { 0.4f, 0.4f, 0.4f };
//	passConstants.lights[2].direction = { 0.0f, -0.707f, -0.707f };
//	passConstants.lights[2].strength = { 0.2f, 0.2f, 0.2f };
//
//	auto currPassCB = mCurrFrameResource->passCB.get();
//	currPassCB->CopyData(0, passConstants);
//
//	UpdateCubeMapFacePassCBs();
//}
//
//void CubeMap::UpdateCubeMapFacePassCBs()
//{
//	for (int i = 0; i < 6; i++)
//	{
//		PassConstants cubeFacePassCB = passConstants;
//
//		XMMATRIX view = mCubeMapCamera[i].GetView();
//		XMMATRIX proj = mCubeMapCamera[i].GetProj();
//
//		XMMATRIX vp = view * proj;
//		XMStoreFloat4x4(&cubeFacePassCB.viewProj, XMMatrixTranspose(vp));
//
//		cubeFacePassCB.eyePosW = mCubeMapCamera[i].GetPosition3f();
//
//		auto currPassCB = mCurrFrameResource->passCB.get();
//		currPassCB->CopyData(1 + i, cubeFacePassCB);
//	}
//}
//
//void CubeMap::LoadTextures()
//{
//	vector<string> texNames = {
//		"bricksTex",
//		"tileTex",
//		"defaultTex",
//		"skyCubeTex"
//	};
//
//	vector<wstring> texFileNames = {
//		L"Textures/bricks2.dds",
//		L"Textures/tile.dds",
//		L"Textures/white1x1.dds",
//		L"Textures/grasscube1024.dds"
//	};
//
//	for (int i = 0; i < texNames.size(); i++)
//	{
//		auto texMap = make_unique<Texture>();
//		texMap->name = texNames[i];
//		texMap->fileName = texFileNames[i];
//		ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), texMap->fileName.c_str(), texMap->resource, texMap->uploadHeap));
//
//		mTextures[texMap->name] = move(texMap);
//	}
//}
//
//void CubeMap::BuildRootSignature()
//{
//	CD3DX12_DESCRIPTOR_RANGE texTable0;
//	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE texTable1;
//	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0);
//
//	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
//	slotRootParameter[0].InitAsConstantBufferView(0);
//	slotRootParameter[1].InitAsConstantBufferView(1);
//	slotRootParameter[2].InitAsShaderResourceView(0, 1);
//	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
//	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
//
//	auto staticSamplers = GetStaticSamplers();
//
//	//根签名由一组根参数构成
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
//		5, //根参数的数量
//		slotRootParameter,
//		(UINT)staticSamplers.size(), //根参数指针
//		staticSamplers.data(),
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//	//用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
//	ComPtr<ID3DBlob> serializedRootSig = nullptr;
//	ComPtr<ID3DBlob> errorBlob = nullptr;
//	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
//		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
//
//	if (errorBlob != nullptr)
//	{
//		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//	}
//	ThrowIfFailed(hr);
//
//	ThrowIfFailed(d3dDevice->CreateRootSignature(
//		0,
//		serializedRootSig->GetBufferPointer(),
//		serializedRootSig->GetBufferSize(),
//		IID_PPV_ARGS(&mRootSignature)));
//}
