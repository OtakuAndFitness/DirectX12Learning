#include "D3D12App.h"
#include "FrameResource.h"
#include "Camera.h"

struct RenderItem {
	RenderItem() = default;

	MeshGeometry* geo = nullptr;
	Material* mat = nullptr;

	//该几何体的世界矩阵
	XMFLOAT4X4 world = MathHelper::Identity4x4();
	//该几何体的顶点UV缩放矩阵
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4();

	int numFramesDirty = frameResourceCount;

	//该几何体的常量数据在objConstantBuffer中的索引
	UINT objCBIndex = -1;

	//该几何体的图元拓扑类型
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	BoundingBox bounds;

	//该几何体的绘制三参数
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	UINT baseVertexLocation = 0;

	bool visible = true;

};

enum class RenderLayer : int
{
	Opaque = 0,
	Highlight,
	Count
};

class Picking : public D3D12App {
public:
	Picking(HINSTANCE hInstance);
	Picking(const Picking& rhs) = delete;
	Picking& operator=(const Picking& rhs) = delete;
	~Picking();

	virtual bool Init()override;

private:
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Draw()override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput();
	void UpdateObjectCBs();
	void UpdateMaterialBuffer();
	void UpdateMainPassCB();

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildPSOs();
	void BuildFrameResource();
	void BuildMaterials();
	void BuildRenderItem();
	void DrawRenderItems(vector<RenderItem*>& items);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
	void Pick(int x, int y);


private:
	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	POINT mLastMousePos;

	vector<unique_ptr<RenderItem>> mAllRenderItems;

	vector<unique_ptr<FrameResource>> mFrameResource;

	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	Camera mCamera;

	unordered_map<string, ComPtr<ID3DBlob>> mShaders;
	unordered_map<string, unique_ptr<Material>> mMaterials;
	unordered_map<string, unique_ptr<Texture>> mTextures;
	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;

	RenderItem* mPickedItem = nullptr;

	vector<RenderItem*> mRenderItemLayer[(int)RenderLayer::Count];
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Picking theApp(hInstance);
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

array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Picking::GetStaticSamplers()
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

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWarp, anisotropicClamp };
}

Picking::Picking(HINSTANCE hInstance) : D3D12App(hInstance)
{
}

Picking::~Picking()
{
	if (d3dDevice != nullptr) {
		FlushCmdQueue();
	}
}

bool Picking::Init()
{
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	mCamera.LookAt(XMFLOAT3(5.0f, 4.0f, -15.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	LoadTextures();
	BuildDescriptorHeaps();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildMaterials();
	BuildRenderItem();
	BuildFrameResource();
	BuildPSOs();

	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void Picking::OnResize()
{
	D3D12App::OnResize();
	//构建投影矩阵
	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void Picking::Update()
{
	OnKeyboardInput();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % frameResourceCount;
	mCurrFrameResource = mFrameResource[mCurrFrameResourceIndex].get();
	if (mCurrFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < mCurrFrameResource->fenceCPU) {
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->fenceCPU, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs();
	UpdateMaterialBuffer();
	UpdateMainPassCB();
}

void Picking::Draw()
{
	auto currCmdAllocator = mCurrFrameResource->cmdAllocator;
	ThrowIfFailed(currCmdAllocator->Reset());
	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSOs["opaque"].Get()));

	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);

	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
	cmdList->ClearRenderTargetView(rtvHandle, Colors::LightBlue, 0, nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->ClearDepthStencilView(dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr);

	cmdList->OMSetRenderTargets(1,
		&rtvHandle,
		true,
		&dsvHandle);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->passCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	auto matBuffer = mCurrFrameResource->materialBuffer->Resource();
	cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootDescriptorTable(3, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Opaque]);

	cmdList->SetPipelineState(mPSOs["highlight"].Get());
	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Highlight]);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(cmdList->Close());

	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	ThrowIfFailed(swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	mCurrFrameResource->fenceCPU = ++mCurrentFence;
	cmdQueue->Signal(fence.Get(), mCurrentFence);
}

void Picking::Pick(int x, int y) {
	XMFLOAT4X4 proj = mCamera.GetProj4x4f();

	float vx = (2.0f * x / mClientWidth - 1) / proj(0, 0);
	float vy = (-2.0f * y / mClientHeight + 1) / proj(1, 1);

	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	mPickedItem->visible = false;

	XMMATRIX view = mCamera.GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	for (auto ri : mRenderItemLayer[(int)RenderLayer::Opaque])
	{
		auto geo = ri->geo;

		if (ri->visible == false) {
			continue;
		}

		XMMATRIX world = XMLoadFloat4x4(&ri->world);
		XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

		XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

		rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
		rayDir = XMVector3TransformNormal(rayDir, toLocal);

		rayDir = XMVector3Normalize(rayDir);

		float tmin = 0.0f;

		if (ri->bounds.Intersects(rayOrigin, rayDir, tmin)) {
			auto vertices = (Vertex*)geo->VertexBufferCPU->GetBufferPointer();
			auto indices = (uint32_t*)geo->IndexBufferCPU->GetBufferPointer();
			UINT triCount = ri->indexCount / 3;

			tmin = MathHelper::Infinity;
			for (UINT i = 0; i < triCount; i++)
			{
				UINT i0 = indices[i * 3 + 0];
				UINT i1 = indices[i * 3 + 1];
				UINT i2 = indices[i * 3 + 2];

				XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Pos);
				XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Pos);
				XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Pos);

				float t = 0.0f;
				if (TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, t)) {
					if (t < tmin) {
						tmin = t;
						UINT pickedTriangle = i;

						mPickedItem->visible = true;
						mPickedItem->indexCount = 3;
						mPickedItem->baseVertexLocation = 0;
						mPickedItem->startIndexLocation = 3 * pickedTriangle;

						mPickedItem->world = ri->world;
						mPickedItem->numFramesDirty = frameResourceCount;

					}
				}
			}
		}
		
	}


}

void Picking::OnMouseDown(WPARAM btnState, int x, int y)
{
	if (btnState & MK_LBUTTON) {
		mLastMousePos.x = x;
		mLastMousePos.y = y;
		SetCapture(mhMainWnd);
		
	}
	else if (btnState & MK_RBUTTON) {
		Pick(x, y);
	}

}

void Picking::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Picking::OnMouseMove(WPARAM btnState, int x, int y)
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

void Picking::OnKeyboardInput()
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

void Picking::UpdateObjectCBs()
{
	auto currObjectCB = mCurrFrameResource->objCB.get();

	for (auto& e : mAllRenderItems)
	{
		if (e->numFramesDirty > 0) {
			XMMATRIX world = XMLoadFloat4x4(&e->world);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.texTransform, XMMatrixTranspose(texTransform));
			objConstants.materialIndex = e->mat->matCBIndex;

			currObjectCB->CopyData(e->objCBIndex, objConstants);

			e->numFramesDirty--;
		}
	}

}

void Picking::UpdateMaterialBuffer()
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

			//将材质常量数据复制到常量缓冲区对应索引地址处
			currMaterialCB->CopyData(mat->matCBIndex, matData);
			//更新下一个帧资源
			mat->numFramesDirty--;
		}
	}
}

void Picking::UpdateMainPassCB()
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX VP_Matrix = XMMatrixMultiply(view, proj);

	PassConstants passConstants;
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

	passConstants.eyePosW = mCamera.GetPosition3f();

	passConstants.ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	passConstants.lights[0].direction = { 0.57735f, -0.57735f, 0.57735f };
	passConstants.lights[0].strength = { 0.8f, 0.8f, 0.8f };
	passConstants.lights[1].direction = { -0.57735f, -0.57735f, 0.57735f };
	passConstants.lights[1].strength = { 0.4f, 0.4f, 0.4f };
	passConstants.lights[2].direction = { 0.0f, -0.707f, -0.707f };
	passConstants.lights[2].strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(0, passConstants);
}

void Picking::LoadTextures()
{
	auto defaultTex = make_unique<Texture>();
	defaultTex->name = "defaultTex";
	defaultTex->fileName = L"Textures/white1x1.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), defaultTex->fileName.c_str(), defaultTex->resource, defaultTex->uploadHeap));

	mTextures[defaultTex->name] = move(defaultTex);

}

void Picking::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		4,
		slotRootParameter,
		(UINT)staticSamplers.size(),
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void Picking::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc,
		IID_PPV_ARGS(&mSrvHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	auto defaultTex = mTextures["defaultTex"]->resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = defaultTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = defaultTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	d3dDevice->CreateShaderResourceView(defaultTex.Get(), &srvDesc, handle);
}

void Picking::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void Picking::BuildGeometry()
{
	ifstream fin("Models/car.txt");

	if (!fin) {
		MessageBox(0, L"Models/car.txt not found", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;

	string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	XMFLOAT3 vMinf3(MathHelper::Infinity, MathHelper::Infinity, MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; i++)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		XMVECTOR p = XMLoadFloat3(&vertices[i].Pos);

		vertices[i].TexC = { 0.0f, 0.0f };

		vMin = XMVectorMin(vMin, p);
		vMax = XMVectorMax(vMax, p);
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
	geo->Name = "carGeo";

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

	SubmeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;
	subMesh.Bounds = bounds;

	geo->DrawArgs[geo->Name] = subMesh;

	mGeometries[geo->Name] = move(geo);
}

void Picking::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaqueDesc.pRootSignature = mRootSignature.Get();
	opaqueDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaqueDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaqueDesc.SampleMask = UINT_MAX;
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	opaqueDesc.SampleDesc.Count = 1;
	opaqueDesc.SampleDesc.Quality = 0;
	opaqueDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaqueDesc;
	transparentPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;//是否开启常规混合（默认值为false）
	transparencyBlendDesc.LogicOpEnable = false;//是否开启逻辑混合(默认值为false)
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//RGB混合中的源混合因子Fsrc（这里取源颜色的alpha通道值）
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//RGB混合中的目标混合因子Fdest（这里取1-alpha）
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;//RGB混合运算符(这里选择加法)
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;//alpha混合中的源混合因子Fsrc（取1）
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;//alpha混合中的目标混合因子Fsrc（取0）
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;//alpha混合运算符(这里选择加法)
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;//逻辑混合运算符(空操作，即不使用)
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//后台缓冲区写入遮罩（没有遮罩，即全部写入）

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc; // 赋值RenderTarget第一个元素，即对每一个渲染目标执行相同操作

	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["highlight"])));
}

void Picking::BuildFrameResource()
{
	for (int i = 0; i < frameResourceCount; i++)
	{
		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
			1,
			(UINT)mAllRenderItems.size(),
			(UINT)mMaterials.size()));
	}
}

void Picking::BuildMaterials()
{
	auto white = make_unique<Material>();
	white->name = "whiteMat";
	white->matCBIndex = 0;
	white->diffuseSrvHeapIndex = 0;
	white->diffuseAlbedo = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	white->fresnelR0 = XMFLOAT3(0.04f, 0.04f, 0.04f);
	white->roughness = 0.0f;

	auto highLight = make_unique<Material>();
	highLight->name = "highLightMat";
	highLight->matCBIndex = 1;
	highLight->diffuseSrvHeapIndex = 0;
	highLight->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.6f);
	highLight->fresnelR0 = XMFLOAT3(0.06f, 0.06f, 0.06f);
	highLight->roughness = 0.0f;

	mMaterials[white->name] = move(white);
	mMaterials[highLight->name] = move(highLight);
}

void Picking::BuildRenderItem()
{
	auto carItem = make_unique<RenderItem>();
	carItem->world = MathHelper::Identity4x4();
	carItem->texTransform = MathHelper::Identity4x4();
	carItem->objCBIndex = 0;
	carItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	carItem->geo = mGeometries["carGeo"].get();
	carItem->mat = mMaterials["whiteMat"].get();
	carItem->indexCount = carItem->geo->DrawArgs["carGeo"].IndexCount;
	carItem->baseVertexLocation = carItem->geo->DrawArgs["carGeo"].BaseVertexLocation;
	carItem->startIndexLocation = carItem->geo->DrawArgs["carGeo"].StartIndexLocation;
	carItem->bounds = carItem->geo->DrawArgs["carGeo"].Bounds;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(carItem.get());

	auto highLightedItem = make_unique<RenderItem>();
	highLightedItem->world = MathHelper::Identity4x4();
	highLightedItem->texTransform = MathHelper::Identity4x4();
	highLightedItem->objCBIndex = 1;
	highLightedItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	highLightedItem->geo = mGeometries["carGeo"].get();
	highLightedItem->mat = mMaterials["highLightMat"].get();
	highLightedItem->indexCount = 0;
	highLightedItem->baseVertexLocation = 0;
	highLightedItem->startIndexLocation = 0;

	highLightedItem->visible = false;

	mPickedItem = highLightedItem.get();
	mRenderItemLayer[(int)RenderLayer::Highlight].push_back(highLightedItem.get());


	mAllRenderItems.push_back(move(carItem));
	mAllRenderItems.push_back(move(highLightedItem));


}

void Picking::DrawRenderItems(vector<RenderItem*>& items)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->objCB->Resource();

	for (size_t i = 0; i < items.size(); i++)
	{
		auto renderItem = items[i];

		if (renderItem->visible == false) {
			continue;
		}

		cmdList->IASetVertexBuffers(0, 1, &renderItem->geo->GetVbv());
		cmdList->IASetIndexBuffer(&renderItem->geo->GetIbv());
		cmdList->IASetPrimitiveTopology(renderItem->primitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + renderItem->objCBIndex * objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(renderItem->indexCount, 1, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
	}
}
