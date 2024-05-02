//#include "D3D12App.h"
//#include "FrameResource.h"
//#include "Camera.h"
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
//	int numFramesDirty = frameResourceCount;
//
//	//该几何体的常量数据在objConstantBuffer中的索引
//	UINT objCBIndex = -1;
//
//	//该几何体的图元拓扑类型
//	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	BoundingBox bounds;
//
//	//该几何体的绘制三参数
//	UINT indexCount = 0;
//	UINT startIndexLocation = 0;
//	UINT baseVertexLocation = 0;
//	UINT instanceCount;
//
//	vector<InstanceData> instances;
//
//
//};
//
//class CPUInstancing : public D3D12App {
//public:
//	CPUInstancing(HINSTANCE hInstance);
//	CPUInstancing(const CPUInstancing& rhs) = delete;
//	CPUInstancing& operator=(const CPUInstancing& rhs) = delete;
//	~CPUInstancing();
//
//	virtual bool Init()override;
//
//private:
//	virtual void OnResize()override;
//	virtual void Update()override;
//	virtual void Draw()override;
//
//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//
//	void OnKeyboardInput();
//	void UpdateInstanceData();
//	void UpdateMaterialBuffer();
//	void UpdateMainPassCB();
//
//	void LoadTextures();
//	void BuildRootSignature();
//	void BuildDescriptorHeaps();
//	void BuildShadersAndInputLayout();
//	void BuildGeometry();
//	void BuildPSOs();
//	void BuildFrameResource();
//	void BuildMaterials();
//	void BuildRenderItem();
//	void DrawRenderItems();
//
//	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
//
//
//private:
//	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
//	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//	
//	POINT mLastMousePos;
//
//	vector<unique_ptr<RenderItem>> mAllRenderItems;
//	vector<RenderItem*> renderItems;
//
//	vector<unique_ptr<FrameResource>> mFrameResource;
//
//	FrameResource* mCurrFrameResource = nullptr;
//	int mCurrFrameResourceIndex = 0;
//	//UINT64 mCurrentFence = 0;//because of this, OnResize() function called then the window freezes
//	bool mFrustumCullingEnbaled = false;
//
//	Camera mCamera;
//
//	UINT mInstanceCount = 0;
//
//	unordered_map<string, ComPtr<ID3DBlob>> mShaders;
//	unordered_map<string, unique_ptr<Material>> mMaterials;
//	unordered_map<string, unique_ptr<Texture>> mTextures;
//	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
//	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;
//};
//
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
//#if defined(DEBUG) || defined(_DEBUG)
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif
//
//	try
//	{
//		CPUInstancing theApp(hInstance);
//		if (!theApp.Init()) {
//			return 0;
//		}
//		return theApp.Run();
//	}
//	catch (DxException& e)
//	{
//		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
//		return 0;
//	}
//
//}
//
//array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CPUInstancing::GetStaticSamplers()
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
//CPUInstancing::CPUInstancing(HINSTANCE hInstance) : D3D12App(hInstance)
//{
//
//}
//
//CPUInstancing::~CPUInstancing()
//{
//	if (d3dDevice != nullptr) {
//		FlushCmdQueue();
//	}
//}
//
//bool CPUInstancing::Init()
//{
//	if (!D3D12App::Init()) {
//		return false;
//	}
//
//	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
//
//	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
//
//	LoadTextures();
//	BuildDescriptorHeaps();
//	BuildRootSignature();
//	BuildShadersAndInputLayout();
//	BuildGeometry();
//	BuildMaterials();
//	BuildRenderItem();
//	BuildFrameResource();
//	BuildPSOs();
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
//void CPUInstancing::OnResize()
//{
//	D3D12App::OnResize();
//	//构建投影矩阵
//	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//}
//
//void CPUInstancing::Update()
//{
//	OnKeyboardInput();
//
//	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % frameResourceCount;
//	mCurrFrameResource = mFrameResource[mCurrFrameResourceIndex].get();
//	if (mCurrFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < mCurrFrameResource->fenceCPU) {
//		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
//		ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->fenceCPU, eventHandle));
//		WaitForSingleObject(eventHandle, INFINITE);
//		CloseHandle(eventHandle);
//	}
//
//	UpdateInstanceData();
//	UpdateMaterialBuffer();
//	UpdateMainPassCB();
//}
//
//void CPUInstancing::Draw()
//{
//	auto currCmdAllocator = mCurrFrameResource->cmdAllocator;
//	ThrowIfFailed(currCmdAllocator->Reset());
//	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSOs["opaque"].Get()));
//
//	cmdList->RSSetViewports(1, &viewPort);
//	cmdList->RSSetScissorRects(1, &scissorRect);
//
//	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
//	cmdList->ClearRenderTargetView(rtvHandle, Colors::LightBlue, 0, nullptr);
//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
//	cmdList->ClearDepthStencilView(dsvHandle, 
//		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
//		1.0f,
//		0, 
//		0, 
//		nullptr);
//
//	cmdList->OMSetRenderTargets(1, 
//		&rtvHandle, 
//		true, 
//		&dsvHandle);
//
//	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
//	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//	
//	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
//
//	auto matBuffer = mCurrFrameResource->materialBuffer->Resource();
//	cmdList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());
//
//	auto passCB = mCurrFrameResource->passCB->Resource();
//	cmdList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
//
//	cmdList->SetGraphicsRootDescriptorTable(3, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
//
//	DrawRenderItems();
//
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//	ThrowIfFailed(cmdList->Close());
//
//	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
//	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
//
//	ThrowIfFailed(swapChain->Present(0, 0));
//	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
//
//	mCurrFrameResource->fenceCPU = ++mCurrentFence;
//	cmdQueue->Signal(fence.Get(), mCurrentFence);
//}
//
//void CPUInstancing::OnMouseDown(WPARAM btnState, int x, int y)
//{
//	mLastMousePos.x = x;
//	mLastMousePos.y = y;
//
//	SetCapture(mhMainWnd);
//}
//
//void CPUInstancing::OnMouseUp(WPARAM btnState, int x, int y)
//{
//	ReleaseCapture();
//}
//
//void CPUInstancing::OnMouseMove(WPARAM btnState, int x, int y)
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
//void CPUInstancing::OnKeyboardInput()
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
//	if (GetAsyncKeyState('1') & 0x8000) {
//		mFrustumCullingEnbaled = true;
//	}
//
//	if (GetAsyncKeyState('2') & 0x8000) {
//		mFrustumCullingEnbaled = false;
//	}
//
//	mCamera.UpdateViewMatrix();
//}
//
//void CPUInstancing::UpdateInstanceData()
//{
//	XMMATRIX view = mCamera.GetView();
//	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
//
//	auto currInstanceBuffer = mCurrFrameResource->instanceBuffer.get();
//
//	for (auto& e : mAllRenderItems) {
//		const auto& instanceData = e->instances;
//		int visibleInstanceCount = 0;
//		for (UINT i = 0; i < (UINT)instanceData.size(); i++)
//		{
//			InstanceData data;
//			XMMATRIX world = XMLoadFloat4x4(&instanceData[i].world);
//			XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].texTransform);
//			XMStoreFloat4x4(&data.world, XMMatrixTranspose(world));
//			XMStoreFloat4x4(&data.texTransform, XMMatrixTranspose(texTransform));
//			
//			XMVECTOR worldDeterminant = XMMatrixDeterminant(world);
//			XMMATRIX invWorld = XMMatrixInverse(&worldDeterminant, world);
//			XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);
//
//			BoundingFrustum localSpaceFrustum;
//			localSpaceFrustum.CreateFromMatrix(localSpaceFrustum, mCamera.GetProj());
//			localSpaceFrustum.Transform(localSpaceFrustum, viewToLocal);
//
//			if (localSpaceFrustum.Contains(e->bounds) != DISJOINT || mFrustumCullingEnbaled == false) {
//				InstanceData data;
//				XMStoreFloat4x4(&data.world, XMMatrixTranspose(world));
//				XMStoreFloat4x4(&data.texTransform, XMMatrixTranspose(texTransform));
//				data.materialIndex = instanceData[i].materialIndex;
//				currInstanceBuffer->CopyData(visibleInstanceCount++, data);
//			}
//			
//			//data.materialIndex = instanceData[i].materialIndex;
//			//currInstanceBuffer->CopyData(visibleInstanceCount++, data);
//		}
//
//		e->instanceCount = visibleInstanceCount;
//
//		wostringstream outs;
//		outs.precision(6);
//		outs << L"Instancing and Culling Demo" << L"    " << e->instanceCount << L" objects visible out of " << e->instances.size();
//		mMainWndCaption = outs.str();
//	}
//}
//
//void CPUInstancing::UpdateMaterialBuffer()
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
//void CPUInstancing::UpdateMainPassCB()
//{
//	XMMATRIX view = mCamera.GetView();
//	XMMATRIX proj = mCamera.GetProj();
//
//	XMMATRIX VP_Matrix = XMMatrixMultiply(view, proj);
//
//	PassConstants passConstants;
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
//}
//
//void CPUInstancing::LoadTextures()
//{
//	auto bricksTex = make_unique<Texture>();
//	bricksTex->name = "bricksTex";
//	bricksTex->fileName = L"Textures/bricks.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), bricksTex->fileName.c_str(), bricksTex->resource, bricksTex->uploadHeap));
//
//	auto stoneTex = make_unique<Texture>();
//	stoneTex->name = "stoneTex";
//	stoneTex->fileName = L"Textures/stone.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), stoneTex->fileName.c_str(), stoneTex->resource, stoneTex->uploadHeap));
//
//	auto tileTex = make_unique<Texture>();
//	tileTex->name = "tileTex";
//	tileTex->fileName = L"Textures/tile.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), tileTex->fileName.c_str(), tileTex->resource, tileTex->uploadHeap));
//
//	auto crateTex = make_unique<Texture>();
//	crateTex->name = "crateTex";
//	crateTex->fileName = L"Textures/WoodCrate01.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), crateTex->fileName.c_str(), crateTex->resource, crateTex->uploadHeap));
//
//	auto iceTex = make_unique<Texture>();
//	iceTex->name = "iceTex";
//	iceTex->fileName = L"Textures/ice.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), iceTex->fileName.c_str(), iceTex->resource, iceTex->uploadHeap));
//
//	auto grassTex = make_unique<Texture>();
//	grassTex->name = "grassTex";
//	grassTex->fileName = L"Textures/grass.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), grassTex->fileName.c_str(), grassTex->resource, grassTex->uploadHeap));
//
//	auto defaultTex = make_unique<Texture>();
//	defaultTex->name = "defaultTex";
//	defaultTex->fileName = L"Textures/white1x1.dds";
//	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), defaultTex->fileName.c_str(), defaultTex->resource, defaultTex->uploadHeap));
//
//	mTextures[bricksTex->name] = move(bricksTex);
//	mTextures[stoneTex->name] = move(stoneTex);
//	mTextures[tileTex->name] = move(tileTex);
//	mTextures[crateTex->name] = move(crateTex);
//	mTextures[iceTex->name] = move(iceTex);
//	mTextures[grassTex->name] = move(grassTex);
//	mTextures[defaultTex->name] = move(defaultTex);
//}
//
//void CPUInstancing::BuildRootSignature()
//{
//	CD3DX12_DESCRIPTOR_RANGE texTable;
//	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0, 0);
//
//	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
//
//	slotRootParameter[0].InitAsShaderResourceView(0, 1);
//	slotRootParameter[1].InitAsShaderResourceView(1, 1);
//	slotRootParameter[2].InitAsConstantBufferView(0);
//	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
//
//	auto staticSamplers = GetStaticSamplers();
//	
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
//		4, 
//		slotRootParameter,
//		(UINT)staticSamplers.size(), 
//		staticSamplers.data(),
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//	
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
//
//void CPUInstancing::BuildDescriptorHeaps()
//{
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 7;
//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	//srvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc,
//		IID_PPV_ARGS(&mSrvHeap)));
//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
//
//	auto bricksTex = mTextures["bricksTex"]->resource;
//	auto stoneTex = mTextures["stoneTex"]->resource;
//	auto tileTex = mTextures["tileTex"]->resource;
//	auto crateTex = mTextures["crateTex"]->resource;
//	auto iceTex = mTextures["iceTex"]->resource;
//	auto grassTex = mTextures["grassTex"]->resource;
//	auto defaultTex = mTextures["defaultTex"]->resource;
//
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
//	srvDesc.Format = stoneTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = tileTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = crateTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = crateTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(crateTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = iceTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = iceTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(iceTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = grassTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = grassTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, handle);
//
//	handle.Offset(1, csuDescriptorSize);
//
//	srvDesc.Format = defaultTex->GetDesc().Format;
//	srvDesc.Texture2D.MipLevels = defaultTex->GetDesc().MipLevels;
//	d3dDevice->CreateShaderResourceView(defaultTex.Get(), &srvDesc, handle);
//}
//
//void CPUInstancing::BuildShadersAndInputLayout()
//{
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Instancing.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Instancing.hlsl", nullptr, "PS", "ps_5_1");
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
//void CPUInstancing::BuildGeometry()
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
//	XMFLOAT3 vMinf3(MathHelper::Infinity, MathHelper::Infinity, MathHelper::Infinity);
//	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
//
//	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
//	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
//
//	vector<Vertex> vertices(vcount);
//	for (UINT i = 0; i < vcount; i++)
//	{
//		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
//		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
//
//		XMVECTOR p = XMLoadFloat3(&vertices[i].Pos);
//
//		XMFLOAT3 spherePos;
//		XMStoreFloat3(&spherePos, XMVector3Normalize(p));
//
//		float theta = atan2f(spherePos.z, spherePos.x);
//		if (theta < 0.0f) {
//			theta += XM_2PI;
//		}
//		float phi = acosf(spherePos.y);
//		float u = theta / (2.0f * XM_PI);
//		float v = phi / XM_PI;
//
//		vertices[i].TexC = { u, v };
//
//		vMin = XMVectorMin(vMin, p);
//		vMax = XMVectorMax(vMax, p);
//	}
//
//	BoundingBox bounds;
//	XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
//	XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));
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
//	geo->Name = "skull";
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
//  skullMesh.Bounds = bounds;
//
//	geo->DrawArgs[geo->Name] = skullMesh;
//
//	mGeometries[geo->Name] = move(geo);
//}
//
//void CPUInstancing::BuildPSOs()
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
//		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
//		mShaders["opaquePS"]->GetBufferSize()
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
//	opaqueDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
//}
//
//void CPUInstancing::BuildFrameResource()
//{
//	for (int i = 0; i < frameResourceCount; i++)
//	{
//		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
//			1, 
//			mInstanceCount,
//			(UINT)mMaterials.size()));
//	}
//}
//
//void CPUInstancing::BuildMaterials()
//{
//	auto bricks = make_unique<Material>();
//	bricks->name = "bricksMat";
//	bricks->matCBIndex = 0;
//	bricks->diffuseSrvHeapIndex = 0;
//	bricks->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	bricks->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
//	bricks->roughness = 0.1f;
//
//	auto stone = make_unique<Material>();
//	stone->name = "stoneMat";
//	stone->matCBIndex = 1;
//	stone->diffuseSrvHeapIndex = 1;
//	stone->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	stone->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
//	stone->roughness = 0.3f;
//
//	auto tile = make_unique<Material>();
//	tile->name = "tileMat";
//	tile->matCBIndex = 2;
//	tile->diffuseSrvHeapIndex = 2;
//	tile->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	tile->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
//	tile->roughness = 0.3f;
//
//	auto crate = make_unique<Material>();
//	crate->name = "crateMat";
//	crate->matCBIndex = 3;
//	crate->diffuseSrvHeapIndex = 3;
//	crate->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	crate->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
//	crate->roughness = 0.2f;
//
//	auto ice = make_unique<Material>();
//	ice->name = "iceMat";
//	ice->matCBIndex = 4;
//	ice->diffuseSrvHeapIndex = 4;
//	ice->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	ice->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	crate->roughness = 0.0f;
//
//	auto grass = make_unique<Material>();
//	grass->name = "grassMat";
//	grass->matCBIndex = 5;
//	grass->diffuseSrvHeapIndex = 5;
//	grass->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	grass->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
//	grass->roughness = 0.2f;
//
//	auto skull = make_unique<Material>();
//	skull->name = "skullMat";
//	skull->matCBIndex = 6;
//	skull->diffuseSrvHeapIndex = 6;
//	skull->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	skull->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
//	skull->roughness = 0.5f;
//
//	mMaterials[bricks->name] = move(bricks);
//	mMaterials[stone->name] = move(stone);
//	mMaterials[tile->name] = move(tile);
//	mMaterials[crate->name] = move(crate);
//	mMaterials[ice->name] = move(ice);
//	mMaterials[grass->name] = move(grass);
//	mMaterials[skull->name] = move(skull);
//
//}
//
//void CPUInstancing::BuildRenderItem()
//{
//	auto skullItem = make_unique<RenderItem>();
//	skullItem->world = MathHelper::Identity4x4();
//	skullItem->texTransform = MathHelper::Identity4x4();
//	skullItem->objCBIndex = 0;
//	skullItem->instanceCount = 0;
//	skullItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	skullItem->geo = mGeometries["skull"].get();
//	skullItem->mat = mMaterials["skullMat"].get();
//	skullItem->indexCount = skullItem->geo->DrawArgs["skull"].IndexCount;
//	skullItem->baseVertexLocation = skullItem->geo->DrawArgs["skull"].BaseVertexLocation;
//	skullItem->startIndexLocation = skullItem->geo->DrawArgs["skull"].StartIndexLocation;
//	skullItem->bounds = skullItem->geo->DrawArgs["skull"].Bounds;
//
//	const int n = 5;
//	mInstanceCount = n * n * n;
//	skullItem->instances.resize(mInstanceCount);
//
//	float width = 200.0f;
//	float height = 200.0f;
//	float depth = 200.0f;
//
//	float x = -0.5f * width;
//	float y = -0.5f * height;
//	float z = -0.5f * depth;
//	float dx = width / (n - 1);
//	float dy = height / (n - 1);
//	float dz = depth / (n - 1);
//
//	for (int k = 0; k< n; k++)
//	{
//		for (int i = 0; i < n; i++) {
//			for (int j = 0; j < n; j++) {
//				int index = k * n * n + i * n + j;
//				skullItem->instances[index].world = XMFLOAT4X4(
//					1.0f, 0.0f, 0.0f, 0.0f,
//					0.0f, 1.0f, 0.0f, 0.0f,
//					0.0f, 0.0f, 1.0f, 0.0f,
//					x + j * dx, y + i * dy, z + k * dz, 1.0f);
//				XMStoreFloat4x4(&skullItem->instances[index].texTransform, XMMatrixScaling(2.0f, 2.0f, 1.0f));
//				skullItem->instances[index].materialIndex = index % mMaterials.size();
//			}
//		}
//		
//	}
//
//	mAllRenderItems.push_back(move(skullItem));
//
//	for (auto& e : mAllRenderItems) {
//		renderItems.push_back(e.get());
//	}
//
//}
//
//void CPUInstancing::DrawRenderItems()
//{
//	for (size_t i = 0; i < renderItems.size(); i++)
//	{
//		auto renderItem = renderItems[i];
//
//		cmdList->IASetVertexBuffers(0, 1, &renderItem->geo->GetVbv());
//		cmdList->IASetIndexBuffer(&renderItem->geo->GetIbv());
//		cmdList->IASetPrimitiveTopology(renderItem->primitiveType);
//
//		auto instanceData = mCurrFrameResource->instanceBuffer->Resource();
//		cmdList->SetGraphicsRootShaderResourceView(0, instanceData->GetGPUVirtualAddress());
//
//		cmdList->DrawIndexedInstanced(renderItem->indexCount, renderItem->instanceCount, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
//	}
//}
