#include "D3D12App.h"
#include "FrameResource.h"
#include "Camera.h"
#include "ShadowMap.h"
#include "SSAO.h"

struct RenderItem {
	RenderItem() = default;

	MeshGeometry* geo = nullptr;
	Material* mat = nullptr;

	//该几何体的世界矩阵
	XMFLOAT4X4 world = MathHelper::Identity4x4();
	//该几何体的顶点UV缩放矩阵
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4();

	int NumFramesDirty = frameResourceCount;

	//该几何体的常量数据在objConstantBuffer中的索引
	UINT objCBIndex = -1;

	//该几何体的图元拓扑类型
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	//该几何体的绘制三参数
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	UINT baseVertexLocation = 0;

};

enum class RenderLayer : int {
	Opaque = 0,
	Debug,
	Sky,
	Count
};

class SSAOApp : public D3D12App {
public:
	SSAOApp(HINSTANCE hInstance);
	SSAOApp(const SSAOApp& rhs) = delete;
	SSAOApp& operator=(const SSAOApp& rhs) = delete;
	~SSAOApp();

	virtual bool Init()override;

private:
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Draw()override;
	virtual void CreateRtvAndDsvDescriptorHeaps()override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput();
	void UpdateObjectCBs();
	void UpdateMaterialBuffer();
	void UpdateMainPassCB();
	void UpdateShadowTransform();
	void UpdateShadowPassCB();
	void UpdateSsao();

	void LoadTextures();
	void BuildRootSignature();
	void BuildSsaoRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildSkull();
	void BuildPSO();
	void BuildFrameResource();
	void BuildMaterials();
	void BuildRenderItem();
	void DrawRenderItems(vector<RenderItem*>& ritems);
	void DrawSceneToShadowMap();
	void DrawNormalsAndDepth();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;


	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();


private:
	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mSsaoRootSignature = nullptr;
	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	POINT mLastMousePos;
	//PassConstants passConstants;

	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f),

	};
	XMFLOAT3 mRotatedLightDirections[3];

	BoundingSphere mSceneBounds;

	XMFLOAT3 mLightPosW;
	//float mLightNearZ = 0.0f;
	//float mLightFarZ = 0.0f;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

	D3D12_GPU_DESCRIPTOR_HANDLE mNullSrv;

	vector<unique_ptr<RenderItem>> mAllRenderItems;

	vector<unique_ptr<FrameResource>> mFrameResource;

	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;
	//UINT64 mCurrentFence = 0;//because of this, OnResize() function called then the window freezes

	Camera mCamera;

	UINT mSkyTexHeapIndex = 0;
	UINT mShadowMapHeapIndex = 0;
	UINT mSsaoHeapIndexStart = 0;
	UINT mSsaoAmbientMapIndex = 0;
	UINT mNullCubeSrvIndex = 0;
	UINT mNullTexSrvIndex1 = 0;
	UINT mNullTexSrvIndex2 = 0;


	unordered_map<string, unique_ptr<Material>> mMaterials;
	unordered_map<string, unique_ptr<Texture>> mTextures;
	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;
	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;

	vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	unique_ptr<ShadowMap> mShadowMap;

	unique_ptr<SSAO> mSsaoMap;

	PassConstants passConstants;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		SSAOApp theApp(hInstance);
		if (!theApp.Init()) {
			return 0;
		}
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}

array<const CD3DX12_STATIC_SAMPLER_DESC, 7> SSAOApp::GetStaticSamplers()
{
	//过滤器POINT,寻址模式WRAP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,//着色器寄存器
		D3D12_FILTER_MIN_MAG_MIP_POINT,//过滤器类型为POINT(常量插值)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,//U方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,//V方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);//W方向上的寻址模式为WRAP（重复寻址模式）

	CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_STATIC_SAMPLER_DESC anisotropicWarp(4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_STATIC_SAMPLER_DESC shadow(6, // 着色器寄存器
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, // U方向上的寻址模式为BORDER
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, // V方向上的寻址模式为BORDER
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, // W方向上的寻址模式为BORDER
		0.0f, // mipLODBias
		16, // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL, //执行阴影图的比较测试
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWarp, anisotropicClamp, shadow };
}

SSAOApp::SSAOApp(HINSTANCE hInstance) : D3D12App(hInstance)
{
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
}

SSAOApp::~SSAOApp()
{
	if (d3dDevice != nullptr) {
		FlushCmdQueue();
	}
}

bool SSAOApp::Init()
{
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	mShadowMap = make_unique<ShadowMap>(d3dDevice.Get(), 2048, 2048);

	mSsaoMap = make_unique<SSAO>(d3dDevice.Get(), cmdList.Get(), mClientWidth, mClientHeight);

	LoadTextures();
	BuildRootSignature();
	BuildSsaoRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildSkull();
	BuildMaterials();
	BuildRenderItem();
	BuildFrameResource();
	BuildPSO();

	mSsaoMap->SetPSOs(mPSOs["ssao"].Get(), mPSOs["ssaoBlur"].Get());

	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void SSAOApp::OnResize()
{
	D3D12App::OnResize();

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	if (mSsaoMap != nullptr) {
		mSsaoMap->OnResize(mClientWidth, mClientHeight);

		mSsaoMap->RebuildDescriptors(mDepthStencilBuffer.Get());
	}
}

void SSAOApp::Update()
{
	OnKeyboardInput();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % frameResourceCount;
	mCurrFrameResource = mFrameResource[mCurrFrameResourceIndex].get();
	//如果GPU端围栏值小于CPU端围栏值，即CPU速度快于GPU，则令CPU等待
	if (mCurrFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < mCurrFrameResource->fenceCPU) {
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->fenceCPU, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	mLightRotationAngle += 0.1f * mTimer.DeltaTime();
	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; i++)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	UpdateObjectCBs();
	UpdateMaterialBuffer();
	UpdateShadowTransform();
	UpdateMainPassCB();
	UpdateShadowPassCB();
	UpdateSsao();
}

void SSAOApp::Draw()
{
	auto currCmdAllocator = mCurrFrameResource->cmdAllocator;

	ThrowIfFailed(currCmdAllocator->Reset());//重复使用记录命令的相关内存

	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSOs["opaque"].Get()));//复用命令列表及其内存

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	////设置根签名
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	auto matBuffer = mCurrFrameResource->materialBuffer->Resource();
	cmdList->SetGraphicsRootShaderResourceView(2, //根参数的起始索引
		matBuffer->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootDescriptorTable(3, mNullSrv);

	cmdList->SetGraphicsRootDescriptorTable(4, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

	DrawSceneToShadowMap();

	DrawNormalsAndDepth();

	cmdList->SetGraphicsRootSignature(mSsaoRootSignature.Get());
	mSsaoMap->ComputeSsao(cmdList.Get(), mCurrFrameResource, 3);

	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	matBuffer = mCurrFrameResource->materialBuffer->Resource();
	cmdList->SetGraphicsRootShaderResourceView(2, //根参数的起始索引
		matBuffer->GetGPUVirtualAddress());

	//设置视口和剪裁矩形
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);

	//转换资源为后台缓冲区资源，从呈现到渲染目标转换
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr); //清除RT背景色为淡蓝，并且不设置裁剪矩形
	//cmdList->ClearDepthStencilView(DepthStencilView(), //DSV描述符句柄
	//	D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, //Flag
	//	1.0f, //默认深度值
	//	0, //默认模板值
	//	0, //裁剪矩形数量
	//	nullptr);//裁剪矩形指针

	cmdList->OMSetRenderTargets(1, //待绑定的RTV数量
		&CurrentBackBufferView(), //指向RTV数组的指针
		true, //RTV对象在堆内存中是连续存放的
		&DepthStencilView());//指向DSV的指针

	cmdList->SetGraphicsRootDescriptorTable(4, mSrvHeap->GetGPUDescriptorHandleForHeapStart());


	auto passCB = mCurrFrameResource->passCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, //根参数的起始索引
		passCB->GetGPUVirtualAddress());

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyTexHeapIndex, csuDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	//绘制顶点（通过索引缓冲区绘制）
	cmdList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);

	cmdList->SetPipelineState(mPSOs["debug"].Get());
	DrawRenderItems(mRitemLayer[(int)RenderLayer::Debug]);

	cmdList->SetPipelineState(mPSOs["sky"].Get());
	DrawRenderItems(mRitemLayer[(int)RenderLayer::Sky]);

	////从渲染目标到呈现
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//完成命令的记录关闭命令列表
	ThrowIfFailed(cmdList->Close());

	ID3D12CommandList* cmdLists[] = { cmdList.Get() };//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);//将命令从命令列表传至命令队列

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;

	//FlushCmdQueue();
	mCurrFrameResource->fenceCPU = ++mCurrentFence;
	cmdQueue->Signal(fence.Get(), mCurrentFence);
}

void SSAOApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void SSAOApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void SSAOApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void SSAOApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		//将鼠标的移动距离换算成弧度，0.25为调节阈值
		float dx = XMConvertToRadians(static_cast<float>(x - mLastMousePos.x) * 0.25f);
		float dy = XMConvertToRadians(static_cast<float>(y - mLastMousePos.y) * 0.25f);
		//计算鼠标没有松开前的累计弧度
		mCamera.Pitch(dy);
		mCamera.Yaw(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void SSAOApp::OnKeyboardInput()
{
	const float dt = mTimer.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000) {
		mCamera.Walk(10.0f * dt);
	}

	if (GetAsyncKeyState('S') & 0x8000) {
		mCamera.Walk(-10.0f * dt);
	}

	if (GetAsyncKeyState('A') & 0x8000) {
		mCamera.Strafe(-10.0f * dt);
	}

	if (GetAsyncKeyState('D') & 0x8000) {
		mCamera.Strafe(10.0f * dt);
	}

	mCamera.UpdateViewMatrix();
}

void SSAOApp::UpdateObjectCBs()
{
	auto currObjectCB = mCurrFrameResource->objCB.get();

	for (auto& e : mAllRenderItems)
	{
		if (e->NumFramesDirty > 0) {
			XMMATRIX world = XMLoadFloat4x4(&e->world);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.texTransform, XMMatrixTranspose(texTransform));
			objConstants.materialIndex = e->mat->matCBIndex;

			currObjectCB->CopyData(e->objCBIndex, objConstants);

			e->NumFramesDirty--;
		}

	}
}

void SSAOApp::UpdateMaterialBuffer()
{
	auto currMaterialCB = mCurrFrameResource->materialBuffer.get();

	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();//获得键值对的值，即Material指针（智能指针转普通指针）
		if (mat->numFramesDirty > 0) {
			//将定义的材质属性传给常量结构体中的元素

			MaterialData matData;
			matData.diffuseAlbedo = mat->diffuseAlbedo;
			matData.fresnelR0 = mat->fresnelR0;
			matData.roughness = mat->roughness;

			XMMATRIX matTransform = XMLoadFloat4x4(&mat->matTransform);
			XMStoreFloat4x4(&matData.matTransform, XMMatrixTranspose(matTransform));
			matData.diffuseMapIndex = mat->diffuseSrvHeapIndex;
			matData.normalMapIndex = mat->normalSrvHeapIndex;

			//将材质常量数据复制到常量缓冲区对应索引地址处
			currMaterialCB->CopyData(mat->matCBIndex, matData);
			//更新下一个帧资源
			mat->numFramesDirty--;
		}
	}
}

void SSAOApp::UpdateMainPassCB()
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX VP_Matrix = XMMatrixMultiply(view, proj);

	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);

	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);
	XMMATRIX viewProjTex = XMMatrixMultiply(VP_Matrix, T);

	XMStoreFloat4x4(&passConstants.shadowTransform, XMMatrixTranspose(shadowTransform));
	XMStoreFloat4x4(&passConstants.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passConstants.proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passConstants.invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passConstants.viewProjTex, XMMatrixTranspose(viewProjTex));

	passConstants.eyePosW = mCamera.GetPosition3f();
	//passConstants.renderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	//passConstants.nearZ = 1.0f;
	//passConstants.farZ = 1000.0f;
	//passConstants.totalTime = mTimer.TotalTime();

	passConstants.ambientLight = { 0.4f, 0.4f, 0.6f, 1.0f };
	passConstants.lights[0].direction = mRotatedLightDirections[0];
	passConstants.lights[0].strength = { 0.4f, 0.4f, 0.5f };
	passConstants.lights[1].direction = mRotatedLightDirections[1];
	passConstants.lights[1].strength = { 0.1f, 0.1f, 0.1f };
	passConstants.lights[2].direction = mRotatedLightDirections[2];
	passConstants.lights[2].strength = { 0.0f, 0.0f, 0.0f };

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(0, passConstants);
}

void SSAOApp::UpdateShadowTransform()
{
	// 主光才投射物体阴影
	XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
	XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	// WorldToLight矩阵 （世界空间转灯光空间）
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mLightPosW, lightPos);//灯光坐标

	// 将包围球变换到光源空间
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// 位于光源空间中包围场景的正交投影视景体
	float l = sphereCenterLS.x - mSceneBounds.Radius;//左端点
	float b = sphereCenterLS.y - mSceneBounds.Radius;//下端点
	float n = sphereCenterLS.z - mSceneBounds.Radius;//近端点
	float r = sphereCenterLS.x + mSceneBounds.Radius;//右端点
	float t = sphereCenterLS.y + mSceneBounds.Radius;//上端点
	float f = sphereCenterLS.z + mSceneBounds.Radius;//远端点

	//mLightNearZ = n;//近裁剪面距离
	//mLightFarZ = f;//远裁剪面距离
	//构建LightToProject矩阵（灯光空间转NDC空间）
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// 构建NDCToTexture矩阵（NDC空间转纹理空间）
	// 从[-1, 1]转到[0, 1]
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);

	// 构建LightToTexture（灯光空间转纹理空间）
	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void SSAOApp::UpdateShadowPassCB()
{
	PassConstants shadowMapPassConstants;

	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	//UINT w = mShadowMap->Width();
	//UINT h = mShadowMap->Height();

	XMStoreFloat4x4(&shadowMapPassConstants.viewProj, XMMatrixTranspose(viewProj));
	//shadowMapPassConstants.eyePosW = mLightPosW;
	//shadowMapPassConstants.renderTargetSize = XMFLOAT2((float)w, (float)h);
	//shadowMapPassConstants.nearZ = mLightNearZ;
	//shadowMapPassConstants.farZ = mLightFarZ;

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(1, shadowMapPassConstants);
}

void SSAOApp::UpdateSsao()
{
	SsaoConstants ssaoCB;

	XMMATRIX P = mCamera.GetProj();

	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);

	ssaoCB.proj = passConstants.proj;
	ssaoCB.invProj = passConstants.invProj;
	XMStoreFloat4x4(&ssaoCB.projTex, XMMatrixTranspose(P * T));

	mSsaoMap->GetOffsetVectors(ssaoCB.offsetVectors);

	auto blurWeights = mSsaoMap->CalcGaussWeights(2.5f);
	ssaoCB.blurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.blurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.blurWeights[2] = XMFLOAT4(&blurWeights[8]);

	ssaoCB.invRenderTargetSize = XMFLOAT2(1.0f / mSsaoMap->Width(), 1.0f / mSsaoMap->Height());

	ssaoCB.occlusionRadius = 0.5f;
	ssaoCB.occlusionFadeStart = 0.2f;
	ssaoCB.occlusionFadeEnd = 1.0f;
	ssaoCB.surfaceEpsilon = 0.05f;

	auto currSsaoCB = mCurrFrameResource->ssaoCB.get();
	currSsaoCB->CopyData(0, ssaoCB);
}

void SSAOApp::LoadTextures()
{
	vector<string> texNames = {
		"bricksTex",
		"bricksNormalTex",
		"tileTex",
		"tileNormalTex",
		"defaultTex",
		"defaultNormalTex",
		"skyCubeTex"
	};

	vector<wstring> texFileNames = {
		L"Textures/bricks2.dds",
		L"Textures/bricks2_nmap.dds",
		L"Textures/tile.dds",
		L"Textures/tile_nmap.dds",
		L"Textures/white1x1.dds",
		L"Textures/default_nmap.dds",
		L"Textures/sunsetcube1024.dds"
	};

	for (int i = 0; i < texNames.size(); i++)
	{
		auto texMap = make_unique<Texture>();
		texMap->name = texNames[i];
		texMap->fileName = texFileNames[i];
		ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), texMap->fileName.c_str(), texMap->resource, texMap->uploadHeap));

		mTextures[texMap->name] = move(texMap);
	}
}

void SSAOApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 3, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		5, //根参数的数量
		slotRootParameter,
		(UINT)staticSamplers.size(), //根参数指针
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	//用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void SSAOApp::BuildSsaoRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstants(1,1);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC pointClamp(0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_STATIC_SAMPLER_DESC linearClamp(1,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_STATIC_SAMPLER_DESC depthMapSam(2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	CD3DX12_STATIC_SAMPLER_DESC linearWrap(3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers = {
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		4, //根参数的数量
		slotRootParameter,
		(UINT)staticSamplers.size(), //根参数指针
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	//用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mSsaoRootSignature.GetAddressOf())));
}

void SSAOApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 18;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc,
		IID_PPV_ARGS(&mSrvHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	vector<ComPtr<ID3D12Resource>> tex2DList = {
		mTextures["bricksTex"]->resource,
		mTextures["bricksNormalTex"]->resource,
		mTextures["tileTex"]->resource,
		mTextures["tileNormalTex"]->resource,
		mTextures["defaultTex"]->resource,
		mTextures["defaultNormalTex"]->resource,
	};

	auto skyCubeTex = mTextures["skyCubeTex"]->resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (UINT i = 0; i < (UINT)tex2DList.size(); i++)
	{
		srvDesc.Format = tex2DList[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
		d3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, handle);

		handle.Offset(1, csuDescriptorSize);

	}

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyCubeTex->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyCubeTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(skyCubeTex.Get(), &srvDesc, handle);

	mSkyTexHeapIndex = (UINT)tex2DList.size();
	mShadowMapHeapIndex = mSkyTexHeapIndex + 1;
	mSsaoHeapIndexStart = mShadowMapHeapIndex + 1;
	mSsaoAmbientMapIndex = mSsaoHeapIndexStart + 3;
	mNullCubeSrvIndex = mSsaoHeapIndexStart + 5;
	mNullTexSrvIndex1 = mNullCubeSrvIndex + 1;
	mNullTexSrvIndex2 = mNullTexSrvIndex1 + 1;

	auto nullSrv = GetCpuSrv(mNullCubeSrvIndex);
	mNullSrv = GetGpuSrv(mNullCubeSrvIndex);
	
	d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	nullSrv.Offset(1, csuDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	nullSrv.Offset(1, csuDescriptorSize);
	d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	mShadowMap->BuildDescriptors(
		GetCpuSrv(mShadowMapHeapIndex),
		GetGpuSrv(mShadowMapHeapIndex),
		GetDsv(1)
	);

	mSsaoMap->BuildDescriptors(mDepthStencilBuffer.Get(), GetCpuSrv(mSsaoHeapIndexStart), GetGpuSrv(mSsaoHeapIndexStart), GetRtv(SwapChainBufferCount), csuDescriptorSize, rtvDescriptorSize);

}

void SSAOApp::BuildShadersAndInputLayout()
{
	/*const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};*/

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["standardPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "PS", "ps_5_1");
	//mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", alphaTestDefines, "PS", "vs_5_1");

	mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["drawNormalVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormal.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["drawNormalPS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormal.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoVS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoPS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

	};
}

void SSAOApp::BuildGeometry()
{
	ProceduralGeometry geoMetry;
	ProceduralGeometry::MeshData box = geoMetry.CreateBox(1.0f, 1.0f, 1.0f, 3);
	ProceduralGeometry::MeshData grid = geoMetry.CreateGrid(20.0f, 30.0f, 60, 40);
	ProceduralGeometry::MeshData sphere = geoMetry.CreateSphere(0.5f, 20, 20);
	ProceduralGeometry::MeshData cylinder = geoMetry.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	ProceduralGeometry::MeshData quad = geoMetry.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	//计算单个几何体顶点在总顶点数组中的偏移量,顺序为：box、grid、sphere、cylinder
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
	UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;
	UINT quadVertexOffset = (UINT)cylinder.Vertices.size() + cylinderVertexOffset;

	//计算单个几何体索引在总索引数组中的偏移量,顺序为：box、grid、sphere、cylinder
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;
	UINT quadIndexOffset = (UINT)cylinder.Indices32.size() + cylinderIndexOffset;

	SubmeshGeometry boxSubMesh;
	boxSubMesh.BaseVertexLocation = boxVertexOffset;
	boxSubMesh.IndexCount = (UINT)box.Indices32.size();
	boxSubMesh.StartIndexLocation = boxIndexOffset;

	SubmeshGeometry gridSubMesh;
	gridSubMesh.BaseVertexLocation = gridVertexOffset;
	gridSubMesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubMesh.StartIndexLocation = gridIndexOffset;

	SubmeshGeometry sphereSubMesh;
	sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
	sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubMesh.StartIndexLocation = sphereIndexOffset;

	SubmeshGeometry cylinderSubMesh;
	cylinderSubMesh.BaseVertexLocation = cylinderVertexOffset;
	cylinderSubMesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubMesh.StartIndexLocation = cylinderIndexOffset;

	SubmeshGeometry quadSubMesh;
	quadSubMesh.BaseVertexLocation = quadVertexOffset;
	quadSubMesh.IndexCount = (UINT)quad.Indices32.size();
	quadSubMesh.StartIndexLocation = quadIndexOffset;

	auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size() + quad.Vertices.size();
	vector<Vertex> vertices(totalVertexCount);//给定顶点数组大小

	int k = 0;
	for (int i = 0; i < box.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
		vertices[k].TangentU = box.Vertices[i].TangentU;
	}

	for (int i = 0; i < grid.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
		vertices[k].TangentU = grid.Vertices[i].TangentU;

	}

	for (int i = 0; i < sphere.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
	}

	for (int i = 0; i < cylinder.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
	}

	for (int i = 0; i < quad.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = quad.Vertices[i].Position;
		vertices[k].Normal = quad.Vertices[i].Normal;
		vertices[k].TexC = quad.Vertices[i].TexC;
		vertices[k].TangentU = quad.Vertices[i].TangentU;
	}

	vector<uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());
	indices.insert(indices.end(), quad.GetIndices16().begin(), quad.GetIndices16().end());

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto mGeo = make_unique<MeshGeometry>();
	mGeo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeo->VertexBufferCPU));
	CopyMemory(mGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeo->IndexBufferCPU));
	CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, mGeo->VertexBufferUploader);

	mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, mGeo->IndexBufferUploader);

	mGeo->VertexByteStride = sizeof(Vertex);
	mGeo->VertexBufferByteSize = vbByteSize;
	mGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mGeo->IndexBufferByteSize = ibByteSize;

	mGeo->DrawArgs["box"] = boxSubMesh;
	mGeo->DrawArgs["grid"] = gridSubMesh;
	mGeo->DrawArgs["sphere"] = sphereSubMesh;
	mGeo->DrawArgs["cylinder"] = cylinderSubMesh;
	mGeo->DrawArgs["quad"] = quadSubMesh;

	mGeometries[mGeo->Name] = move(mGeo);
}

void SSAOApp::BuildSkull()
{
	ifstream fin("Models/skull.txt");

	if (!fin) {
		MessageBox(0, L"Models/skull.txt not found", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;

	string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; i++)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		vertices[i].TexC = { 0.0f, 0.0f };

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);
		XMVECTOR N = XMLoadFloat3(&vertices[i].Normal);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f) {
			XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
			XMStoreFloat3(&vertices[i].TangentU, T);
		}
		else {
			up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
			XMStoreFloat3(&vertices[i].TangentU, T);
		}

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	BoundingBox bounds;
	XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
	XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	vector<int32_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; i++)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(int32_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));//创建顶点数据内存空间
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);//将顶点数据拷贝至顶点系统内存中

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));//创建索引数据内存空间
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);//将索引数据拷贝至索引系统内存中

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry skullMesh;
	skullMesh.IndexCount = (UINT)indices.size();
	skullMesh.StartIndexLocation = 0;
	skullMesh.BaseVertexLocation = 0;

	geo->DrawArgs[geo->Name] = skullMesh;

	mGeometries[geo->Name] = move(geo);
}

void SSAOApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC baseDesc;
	ZeroMemory(&baseDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	baseDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	baseDesc.pRootSignature = mRootSignature.Get();
	baseDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	baseDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardPS"]->GetBufferPointer()),
		mShaders["standardPS"]->GetBufferSize()
	};
	baseDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	baseDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	baseDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	baseDesc.SampleMask = UINT_MAX;
	baseDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	baseDesc.NumRenderTargets = 1;
	baseDesc.RTVFormats[0] = mBackBufferFormat;
	baseDesc.SampleDesc.Count = 1;
	baseDesc.SampleDesc.Quality = 0;
	baseDesc.DSVFormat = mDepthStencilFormat;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc = baseDesc;
	opaqueDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	opaqueDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapDesc = baseDesc;
	shadowMapDesc.RasterizerState.DepthBias = 100000;
	shadowMapDesc.RasterizerState.DepthBiasClamp = 0.0f;
	shadowMapDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	shadowMapDesc.pRootSignature = mRootSignature.Get();
	shadowMapDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
		mShaders["shadowVS"]->GetBufferSize()
	};
	shadowMapDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()),
		mShaders["shadowOpaquePS"]->GetBufferSize()
	};
	shadowMapDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	shadowMapDesc.NumRenderTargets = 0;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&shadowMapDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugDesc = baseDesc;
	debugDesc.pRootSignature = mRootSignature.Get();
	debugDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
		mShaders["debugVS"]->GetBufferSize()
	};
	debugDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
		mShaders["debugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&debugDesc, IID_PPV_ARGS(&mPSOs["debug"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalDesc = baseDesc;
	drawNormalDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["drawNormalVS"]->GetBufferPointer()),
		mShaders["drawNormalVS"]->GetBufferSize()
	};
	drawNormalDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["drawNormalPS"]->GetBufferPointer()),
		mShaders["drawNormalPS"]->GetBufferSize()
	};
	drawNormalDesc.RTVFormats[0] = SSAO::normalMapFormat;
	drawNormalDesc.SampleDesc.Count = 1;
	drawNormalDesc.SampleDesc.Quality = 0;
	drawNormalDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&drawNormalDesc, IID_PPV_ARGS(&mPSOs["drawNormal"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoDesc = baseDesc;
	ssaoDesc.InputLayout = { nullptr,0 };
	ssaoDesc.pRootSignature = mSsaoRootSignature.Get();
	ssaoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["ssaoVS"]->GetBufferPointer()),
		mShaders["ssaoVS"]->GetBufferSize()
	};
	ssaoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["ssaoPS"]->GetBufferPointer()),
		mShaders["ssaoPS"]->GetBufferSize()
	};
	ssaoDesc.DepthStencilState.DepthEnable = false;
	ssaoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoDesc.RTVFormats[0] = SSAO::ambientMapFormat;
	ssaoDesc.SampleDesc.Count = 1;
	ssaoDesc.SampleDesc.Quality = 0;
	ssaoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssaoDesc, IID_PPV_ARGS(&mPSOs["ssao"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurDesc = ssaoDesc;
	ssaoBlurDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurVS"]->GetBufferPointer()),
		mShaders["ssaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurPS"]->GetBufferPointer()),
		mShaders["ssaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssaoBlurDesc, IID_PPV_ARGS(&mPSOs["ssaoBlur"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = baseDesc;
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = mRootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));
}

void SSAOApp::BuildFrameResource()
{
	for (int i = 0; i < frameResourceCount; i++)
	{
		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
			2, //passCount
			(UINT)mAllRenderItems.size(),//objCount
			(UINT)mMaterials.size()));
	}
}

void SSAOApp::BuildMaterials()
{
	auto bricks = make_unique<Material>();
	bricks->name = "bricksMat";
	bricks->matCBIndex = 0;
	bricks->diffuseSrvHeapIndex = 0;
	bricks->normalSrvHeapIndex = 1;
	bricks->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks->roughness = 0.3f;

	auto tile = make_unique<Material>();
	tile->name = "tileMat";
	tile->matCBIndex = 2;
	tile->diffuseSrvHeapIndex = 2;
	tile->normalSrvHeapIndex = 3;
	tile->diffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile->fresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tile->roughness = 0.1f;

	auto mirror = make_unique<Material>();
	mirror->name = "mirrorMat";
	mirror->matCBIndex = 3;
	mirror->diffuseSrvHeapIndex = 4;
	mirror->normalSrvHeapIndex = 5;
	mirror->diffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.1f, 1.0f);
	mirror->fresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirror->roughness = 0.1f;

	auto skull = make_unique<Material>();
	skull->name = "skullMat";
	skull->matCBIndex = 3;
	skull->diffuseSrvHeapIndex = 4;
	skull->normalSrvHeapIndex = 5;
	skull->diffuseAlbedo = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	skull->fresnelR0 = XMFLOAT3(0.6f, 0.6f, 0.6f);
	skull->roughness = 0.2f;

	auto sky = make_unique<Material>();
	sky->name = "skyMat";
	sky->matCBIndex = 4;
	sky->diffuseSrvHeapIndex = 6;
	sky->normalSrvHeapIndex = 7;
	sky->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->roughness = 1.0f;

	mMaterials[bricks->name] = move(bricks);
	mMaterials[mirror->name] = move(mirror);
	mMaterials[tile->name] = move(tile);
	mMaterials[sky->name] = move(sky);
	mMaterials[skull->name] = move(skull);
}

void SSAOApp::BuildRenderItem()
{
	auto skyRenderItem = make_unique<RenderItem>();
	XMStoreFloat4x4(&(skyRenderItem->world), XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRenderItem->texTransform = MathHelper::Identity4x4();
	skyRenderItem->objCBIndex = 0;
	skyRenderItem->mat = mMaterials["skyMat"].get();
	skyRenderItem->geo = mGeometries["shapeGeo"].get();
	skyRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRenderItem->indexCount = skyRenderItem->geo->DrawArgs["sphere"].IndexCount;
	skyRenderItem->baseVertexLocation = skyRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
	skyRenderItem->startIndexLocation = skyRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Sky].push_back(skyRenderItem.get());
	mAllRenderItems.push_back(move(skyRenderItem));

	auto quadRenderItem = make_unique<RenderItem>();
	quadRenderItem->world = MathHelper::Identity4x4();
	quadRenderItem->texTransform = MathHelper::Identity4x4();
	quadRenderItem->objCBIndex = 1;
	quadRenderItem->mat = mMaterials["bricksMat"].get();
	quadRenderItem->geo = mGeometries["shapeGeo"].get();
	quadRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quadRenderItem->indexCount = quadRenderItem->geo->DrawArgs["quad"].IndexCount;
	quadRenderItem->baseVertexLocation = quadRenderItem->geo->DrawArgs["quad"].BaseVertexLocation;
	quadRenderItem->startIndexLocation = quadRenderItem->geo->DrawArgs["quad"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Debug].push_back(quadRenderItem.get());
	mAllRenderItems.push_back(move(quadRenderItem));

	auto boxRenderItem = make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRenderItem->world, XMMatrixScaling(2.0f, 1.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	XMStoreFloat4x4(&boxRenderItem->texTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	boxRenderItem->objCBIndex = 2;
	boxRenderItem->mat = mMaterials["bricksMat"].get();
	boxRenderItem->geo = mGeometries["shapeGeo"].get();
	boxRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRenderItem->indexCount = boxRenderItem->geo->DrawArgs["box"].IndexCount;
	boxRenderItem->baseVertexLocation = boxRenderItem->geo->DrawArgs["box"].BaseVertexLocation;
	boxRenderItem->startIndexLocation = boxRenderItem->geo->DrawArgs["box"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRenderItem.get());
	mAllRenderItems.push_back(move(boxRenderItem));

	auto skullRenderItem = make_unique<RenderItem>();
	XMStoreFloat4x4(&skullRenderItem->world, XMMatrixScaling(0.4f, 0.4f, 0.4f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRenderItem->texTransform = MathHelper::Identity4x4();
	skullRenderItem->objCBIndex = 3;
	skullRenderItem->mat = mMaterials["skullMat"].get();
	skullRenderItem->geo = mGeometries["skullGeo"].get();
	skullRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->indexCount = skullRenderItem->geo->DrawArgs["skullGeo"].IndexCount;
	skullRenderItem->baseVertexLocation = skullRenderItem->geo->DrawArgs["skullGeo"].BaseVertexLocation;
	skullRenderItem->startIndexLocation = skullRenderItem->geo->DrawArgs["skullGeo"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());
	mAllRenderItems.push_back(move(skullRenderItem));

	auto gridRenderItem = make_unique<RenderItem>();
	gridRenderItem->world = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRenderItem->texTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRenderItem->objCBIndex = 4;
	gridRenderItem->mat = mMaterials["tileMat"].get();
	gridRenderItem->geo = mGeometries["shapeGeo"].get();
	gridRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->indexCount = gridRenderItem->geo->DrawArgs["grid"].IndexCount;
	gridRenderItem->baseVertexLocation = gridRenderItem->geo->DrawArgs["grid"].BaseVertexLocation;
	gridRenderItem->startIndexLocation = gridRenderItem->geo->DrawArgs["grid"].StartIndexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());
	mAllRenderItems.push_back(move(gridRenderItem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
	UINT followObjCBIndex = 5;//接下去的几何体常量数据在CB中的索引从2开始
	//将圆柱和圆的实例模型存入渲染项中
	for (int i = 0; i < 5; i++)
	{
		auto leftCylinderRenderItem = make_unique<RenderItem>();
		auto rightCylinderRenderItem = make_unique<RenderItem>();
		auto leftSphereRenderItem = make_unique<RenderItem>();
		auto rightSphereRenderItem = make_unique<RenderItem>();

		XMMATRIX leftCylinderWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylinderWorld = XMMatrixTranslation(5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(5.0f, 3.5f, -10.0f + i * 5.0f);

		//左边5个圆柱
		XMStoreFloat4x4(&(leftCylinderRenderItem->world), leftCylinderWorld);
		XMStoreFloat4x4(&(leftCylinderRenderItem->texTransform), brickTexTransform);
		//此处的索引随着循环不断加1（注意：这里是先赋值再++）
		leftCylinderRenderItem->objCBIndex = followObjCBIndex++;
		leftCylinderRenderItem->mat = mMaterials["bricksMat"].get();
		leftCylinderRenderItem->geo = mGeometries["shapeGeo"].get();
		leftCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylinderRenderItem->indexCount = leftCylinderRenderItem->geo->DrawArgs["cylinder"].IndexCount;
		leftCylinderRenderItem->baseVertexLocation = leftCylinderRenderItem->geo->DrawArgs["cylinder"].BaseVertexLocation;
		leftCylinderRenderItem->startIndexLocation = leftCylinderRenderItem->geo->DrawArgs["cylinder"].StartIndexLocation;
		//右边5个圆柱
		XMStoreFloat4x4(&(rightCylinderRenderItem->world), rightCylinderWorld);
		XMStoreFloat4x4(&(rightCylinderRenderItem->texTransform), brickTexTransform);
		rightCylinderRenderItem->objCBIndex = followObjCBIndex++;
		rightCylinderRenderItem->mat = mMaterials["bricksMat"].get();
		rightCylinderRenderItem->geo = mGeometries["shapeGeo"].get();
		rightCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylinderRenderItem->indexCount = rightCylinderRenderItem->geo->DrawArgs["cylinder"].IndexCount;
		rightCylinderRenderItem->baseVertexLocation = rightCylinderRenderItem->geo->DrawArgs["cylinder"].BaseVertexLocation;
		rightCylinderRenderItem->startIndexLocation = rightCylinderRenderItem->geo->DrawArgs["cylinder"].StartIndexLocation;
		//左边5个球
		XMStoreFloat4x4(&(leftSphereRenderItem->world), leftSphereWorld);
		leftSphereRenderItem->texTransform = MathHelper::Identity4x4();
		leftSphereRenderItem->objCBIndex = followObjCBIndex++;
		leftSphereRenderItem->mat = mMaterials["mirrorMat"].get();
		leftSphereRenderItem->geo = mGeometries["shapeGeo"].get();
		leftSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRenderItem->indexCount = leftSphereRenderItem->geo->DrawArgs["sphere"].IndexCount;
		leftSphereRenderItem->baseVertexLocation = leftSphereRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
		leftSphereRenderItem->startIndexLocation = leftSphereRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
		//右边5个球
		XMStoreFloat4x4(&(rightSphereRenderItem->world), rightSphereWorld);
		rightSphereRenderItem->texTransform = MathHelper::Identity4x4();
		rightSphereRenderItem->mat = mMaterials["mirrorMat"].get();
		rightSphereRenderItem->geo = mGeometries["shapeGeo"].get();
		rightSphereRenderItem->objCBIndex = followObjCBIndex++;
		rightSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRenderItem->indexCount = rightSphereRenderItem->geo->DrawArgs["sphere"].IndexCount;
		rightSphereRenderItem->baseVertexLocation = rightSphereRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
		rightSphereRenderItem->startIndexLocation = rightSphereRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;

		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylinderRenderItem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylinderRenderItem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRenderItem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRenderItem.get());

		mAllRenderItems.push_back(move(leftCylinderRenderItem));
		mAllRenderItems.push_back(move(rightCylinderRenderItem));
		mAllRenderItems.push_back(move(leftSphereRenderItem));
		mAllRenderItems.push_back(move(rightSphereRenderItem));

	}
}

void SSAOApp::DrawRenderItems(vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->objCB->Resource();

	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto renderItem = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &renderItem->geo->GetVbv());
		cmdList->IASetIndexBuffer(&renderItem->geo->GetIbv());
		cmdList->IASetPrimitiveTopology(renderItem->primitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + renderItem->objCBIndex * objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(renderItem->indexCount, 1, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
	}
}

void SSAOApp::DrawSceneToShadowMap()
{
	cmdList->RSSetViewports(1, &mShadowMap->Viewport());
	cmdList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	cmdList->ClearDepthStencilView(mShadowMap->Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());

	auto passCB = mCurrFrameResource->passCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + passCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	cmdList->SetPipelineState(mPSOs["shadow_opaque"].Get());
	DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void SSAOApp::DrawNormalsAndDepth()
{
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);

	auto normalMap = mSsaoMap->NormalMap();
	auto normalMapRtv = mSsaoMap->NormalMapRtv();

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 0.0f,0.0f,1.0f,0.0f };
	cmdList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	cmdList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

	auto passCB = mCurrFrameResource->passCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	cmdList->SetPipelineState(mPSOs["drawNormal"].Get());
	DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SSAOApp::GetCpuSrv(int index) const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, csuDescriptorSize);

	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SSAOApp::GetGpuSrv(int index) const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, csuDescriptorSize);

	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SSAOApp::GetDsv(int index) const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, dsvDescriptorSize);

	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SSAOApp::GetRtv(int index) const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, rtvDescriptorSize);

	return rtv;
}
