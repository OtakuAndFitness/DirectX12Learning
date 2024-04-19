#include "D3D12App.h"
#include "FrameResource.h"

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

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	Count
};

class StencilDemo : public D3D12App {
public:
	StencilDemo(HINSTANCE hInstance);
	StencilDemo(const StencilDemo& rhs) = delete;
	StencilDemo& operator=(const StencilDemo& rhs) = delete;
	~StencilDemo();
	virtual bool Init()override;

private:
	void BuildDescriptorHeaps();
	void BuildRoomGeometry();
	void BuildSkullGeometry();
	void BuildRootSignature();
	void BuildPSOs();
	void BuildShadersAndInputLayout();
	void BuildRenderItems();
	void DrawRenderItems(vector<RenderItem*>& items);
	void BuildFrameResource();
	void BuildMaterials();
	void LoadTextures();

	void OnKeyboardInput();
	void UpdateCamera();
	void UpdateObjectCBs();
	void UpdateMainPassCB();
	void UpdateMatCBs();

	virtual void Draw()override;
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	POINT mLastMousePos;

	float mTheta = 1.24f * XM_PI;
	float mPhi = 0.42f * XM_PIDIV2;
	float mRadius = 12.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	XMFLOAT3 mSkullTranslation = { 4.0f,0.0f,0.0f };

	
	int mCurrFrameResourceIndex = 0;
	FrameResource* mCurrFrameResource = nullptr;
	vector<unique_ptr<FrameResource>> mFrameResource;

	vector<unique_ptr<RenderItem>> mAllRenderItems;
	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
	unordered_map<string, unique_ptr<Texture>> mTextures;
	unordered_map<string, unique_ptr<Material>> mMaterials;

	ComPtr<ID3D12DescriptorHeap> mSrcDescriptorHeap = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	unordered_map<string, ComPtr<ID3DBlob>> mShaders;

	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;


	vector<RenderItem*> mRenderItemLayer[(int)RenderLayer::Count];

	unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		StencilDemo theApp(hInstance);
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

StencilDemo::StencilDemo(HINSTANCE hInstance) : D3D12App(hInstance) {

}

StencilDemo::~StencilDemo() {
	if (d3dDevice != nullptr) {
		FlushCmdQueue();
	}
}

bool StencilDemo::Init() {
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildRoomGeometry();
	BuildSkullGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResource();
	BuildPSOs();

	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void StencilDemo::LoadTextures() {
	auto skullTex = make_unique<Texture>();
	skullTex->name = "skullTex";
	skullTex->fileName = L"Textures/white1x1.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), skullTex->fileName.c_str(), skullTex->resource, skullTex->uploadHeap));

	auto floorTex = make_unique<Texture>();
	floorTex->name = "floorTex";
	floorTex->fileName = L"Textures/checkboard.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), floorTex->fileName.c_str(), floorTex->resource, floorTex->uploadHeap));

	auto wallTex = make_unique<Texture>();
	wallTex->name = "wallTex";
	wallTex->fileName = L"Textures/bricks3.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), wallTex->fileName.c_str(), wallTex->resource, wallTex->uploadHeap));

	auto mirrorTex = make_unique<Texture>();
	mirrorTex->name = "mirrorTex";
	mirrorTex->fileName = L"Textures/ice.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), mirrorTex->fileName.c_str(), mirrorTex->resource, mirrorTex->uploadHeap));

	mTextures[skullTex->name] = move(skullTex);
	mTextures[floorTex->name] = move(floorTex);
	mTextures[wallTex->name] = move(wallTex);
	mTextures[mirrorTex->name] = move(mirrorTex);
}

void StencilDemo::BuildRootSignature() {
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	//根参数可以是描述符表、根描述符、根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(4, //根参数的数量
		slotRootParameter, //根参数指针
		(UINT)staticSamplers.size(),
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	//用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSig, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

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

void StencilDemo::BuildDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = 4;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrcDescriptorHeap)));

	auto skullTex = mTextures["skullTex"]->resource;
	auto floorTex = mTextures["floorTex"]->resource;
	auto wallTex = mTextures["wallTex"]->resource;
	auto mirrorTex = mTextures["mirrorTex"]->resource;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrcDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = floorTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	d3dDevice->CreateShaderResourceView(floorTex.Get(), &srvDesc, handle);

	handle.Offset(1, csuDescriptorSize);

	srvDesc.Format = wallTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(wallTex.Get(), &srvDesc, handle);

	handle.Offset(1, csuDescriptorSize);

	srvDesc.Format = skullTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(skullTex.Get(), &srvDesc, handle);

	handle.Offset(1, csuDescriptorSize);

	srvDesc.Format = mirrorTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(mirrorTex.Get(), &srvDesc, handle);
}

void StencilDemo::BuildShadersAndInputLayout() {

	const D3D_SHADER_MACRO defines[] = {
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] = {
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", defines, "PS", "ps_5_0");
	mShaders["alphaTestPS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", alphaTestDefines, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
}

void StencilDemo::BuildRoomGeometry() {
	std::array<Vertex, 20> vertices =
	{
		// Floor: Observe we tile texture coordinates.
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	std::array<std::int16_t, 30> indices =
	{
		// Floor
		0, 1, 2,
		0, 2, 3,

		// Walls
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// Mirror
		16, 17, 18,
		16, 18, 19
	};

	SubmeshGeometry floorMesh;
	floorMesh.IndexCount = 6;
	floorMesh.StartIndexLocation = 0;
	floorMesh.BaseVertexLocation = 0;

	SubmeshGeometry wallMesh;
	wallMesh.IndexCount = 18;
	wallMesh.StartIndexLocation = 6;
	wallMesh.BaseVertexLocation = 0;

	SubmeshGeometry mirrorMesh;
	mirrorMesh.IndexCount = 6;
	mirrorMesh.StartIndexLocation = 24;
	mirrorMesh.BaseVertexLocation = 0;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "room";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));//创建顶点数据内存空间
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);//将顶点数据拷贝至顶点系统内存中

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));//创建索引数据内存空间
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);//将索引数据拷贝至索引系统内存中

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["floor"] = floorMesh;
	geo->DrawArgs["wall"] = wallMesh;
	geo->DrawArgs["mirror"] = mirrorMesh;

	mGeometries["room"] = move(geo);

}

void StencilDemo::BuildSkullGeometry() {
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

	vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; i++)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		vertices[i].TexC = { 0.0f, 0.0f };
	}

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
	geo->Name = "skull";

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

void StencilDemo::BuildMaterials() {
	auto floor = make_unique<Material>();
	floor->name = "floorMat";
	floor->matCBIndex = 0;
	floor->diffuseSrvHeapIndex = 0;
	floor->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	floor->fresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	floor->roughness = 0.3f;

	auto wall = make_unique<Material>();
	wall->name = "wallMat";
	wall->matCBIndex = 1;
	wall->diffuseSrvHeapIndex = 1;
	wall->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wall->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	wall->roughness = 0.25f;

	auto skull = make_unique<Material>();
	skull->name = "skullMat";
	skull->matCBIndex = 2;
	skull->diffuseSrvHeapIndex = 2;
	skull->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skull->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skull->roughness = 0.3f;

	auto mirror = make_unique<Material>();
	mirror->name = "mirrorMat";
	mirror->matCBIndex = 3;
	mirror->diffuseSrvHeapIndex = 3;
	mirror->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	mirror->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	mirror->roughness = 0.5f;

	mMaterials[floor->name] = move(floor);
	mMaterials[wall->name] = move(wall);
	mMaterials[mirror->name] = move(mirror);
	mMaterials[skull->name] = move(skull);
}

void StencilDemo::BuildRenderItems() {
	auto floorItem = make_unique<RenderItem>();
	//floorItem->world = MathHelper::Identity4x4();
	XMStoreFloat4x4(&(floorItem->world), XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f) * XMMatrixRotationX(0.5f * MathHelper::Pi));
	//XMStoreFloat4x4(&floorItem->texTransform, XMMatrixScaling(1.6f, 1.6f, 1.6f));
	floorItem->texTransform = MathHelper::Identity4x4();
	floorItem->objCBIndex = 0;
	floorItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorItem->geo = mGeometries["room"].get();
	floorItem->mat = mMaterials["floorMat"].get();
	floorItem->indexCount = floorItem->geo->DrawArgs["floor"].IndexCount;
	floorItem->baseVertexLocation = floorItem->geo->DrawArgs["floor"].BaseVertexLocation;
	floorItem->startIndexLocation = floorItem->geo->DrawArgs["floor"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(floorItem.get());

	auto wallItem = make_unique<RenderItem>();
	//wallItem->world = MathHelper::Identity4x4();
	wallItem->texTransform = MathHelper::Identity4x4();
	XMStoreFloat4x4(&(wallItem->world), XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f) * XMMatrixRotationX(-0.5f * MathHelper::Pi));
	//XMStoreFloat4x4(&wallItem->texTransform, XMMatrixScaling(0.8f, 0.8f, 0.8f));
	wallItem->objCBIndex = 1;
	wallItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallItem->geo = mGeometries["room"].get();
	wallItem->mat = mMaterials["wallMat"].get();
	wallItem->indexCount = wallItem->geo->DrawArgs["wall"].IndexCount;
	wallItem->baseVertexLocation = wallItem->geo->DrawArgs["wall"].BaseVertexLocation;
	wallItem->startIndexLocation = wallItem->geo->DrawArgs["wall"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(wallItem.get());

	auto skullItem = make_unique<RenderItem>();
	//skullItem->world = MathHelper::Identity4x4();
	XMStoreFloat4x4(&skullItem->world, XMMatrixScaling(0.45f, 0.45f, 0.45f) * XMMatrixTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z) * XMMatrixRotationY(0.5f * MathHelper::Pi));
	skullItem->texTransform = MathHelper::Identity4x4();
	skullItem->objCBIndex = 2;
	skullItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullItem->geo = mGeometries["skull"].get();
	skullItem->mat = mMaterials["skullMat"].get();
	skullItem->indexCount = skullItem->geo->DrawArgs["skull"].IndexCount;
	skullItem->baseVertexLocation = skullItem->geo->DrawArgs["skull"].BaseVertexLocation;
	skullItem->startIndexLocation = skullItem->geo->DrawArgs["skull"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(skullItem.get());

	auto mirrorItem = make_unique<RenderItem>();
	//mirrorItem->world = MathHelper::Identity4x4();
	XMStoreFloat4x4(&(mirrorItem->world), XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f) * XMMatrixRotationX(-0.5f * MathHelper::Pi));
	mirrorItem->texTransform = MathHelper::Identity4x4();
	//XMStoreFloat4x4(&mirrorItem->texTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	mirrorItem->objCBIndex = 3;
	mirrorItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorItem->geo = mGeometries["room"].get();
	mirrorItem->mat = mMaterials["mirrorMat"].get();
	mirrorItem->indexCount = mirrorItem->geo->DrawArgs["mirror"].IndexCount;
	mirrorItem->baseVertexLocation = mirrorItem->geo->DrawArgs["mirror"].BaseVertexLocation;
	mirrorItem->startIndexLocation = mirrorItem->geo->DrawArgs["mirror"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(mirrorItem.get());

	mAllRenderItems.push_back(move(floorItem));
	mAllRenderItems.push_back(move(wallItem));
	mAllRenderItems.push_back(move(skullItem));
	mAllRenderItems.push_back(move(mirrorItem));
}

void StencilDemo::BuildFrameResource() {
	for (int i = 0; i < frameResourceCount; i++)
	{
		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
			1, //passCount
			(UINT)mAllRenderItems.size(),//objCount
			(UINT)mMaterials.size()));
	}
}

void StencilDemo::BuildPSOs() {
	//不透明物体的PSO（不需要混合）
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

	//半透明物体的PSO（需要混合）
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaqueDesc;
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

	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));
}

void StencilDemo::DrawRenderItems(vector<RenderItem*>& items) {
	UINT objConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MatConstants));

	auto objCB = mCurrFrameResource->objCB->Resource();
	auto matCB = mCurrFrameResource->matCB->Resource();


	for (size_t i = 0; i < items.size(); i++)
	{
		auto ri = items[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->geo->GetVbv());
		cmdList->IASetIndexBuffer(&ri->geo->GetIbv());
		cmdList->IASetPrimitiveTopology(ri->primitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(mSrcDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		texHandle.Offset(ri->mat->diffuseSrvHeapIndex, csuDescriptorSize);

		//设置根描述符,将根描述符与资源绑定
		auto objCBAddress = objCB->GetGPUVirtualAddress();
		auto matCBAddress = matCB->GetGPUVirtualAddress();

		objCBAddress += ri->objCBIndex * objConstSize;
		matCBAddress += ri->mat->matCBIndex * matConstSize;

		cmdList->SetGraphicsRootDescriptorTable(0, texHandle);
		cmdList->SetGraphicsRootConstantBufferView(1, //寄存器槽号
			objCBAddress);//子资源地址
		//绘制顶点（通过索引缓冲区绘制）
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->indexCount, //每个实例要绘制的索引数
			1, //实例化个数
			ri->startIndexLocation, //起始索引位置
			ri->baseVertexLocation, //子物体起始索引在全局索引中的位置
			0);//实例化的高级技术，暂时设置为0
	}
}

void StencilDemo::Draw() {
	auto currCmdAllocator = mCurrFrameResource->cmdAllocator;
	ThrowIfFailed(currCmdAllocator->Reset());//重复使用记录命令的相关内存
	//if (mIsWireframe) {
		//ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSOs["opaque_wireframe"].Get()));//复用命令列表及其内存
	//}
	//else {
	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSOs["opaque"].Get()));//复用命令列表及其内存
	//}

	//设置视口和剪裁矩形
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);

	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
	//转换资源为后台缓冲区资源，从呈现到渲染目标转换
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
	cmdList->ClearRenderTargetView(rtvHandle, Colors::LightBlue, 0, nullptr); //清除RT背景色为淡蓝，并且不设置裁剪矩形
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->ClearDepthStencilView(dsvHandle, //DSV描述符句柄
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, //Flag
		1.0f, //默认深度值
		0, //默认模板值
		0, //裁剪矩形数量
		nullptr);//裁剪矩形指针

	cmdList->OMSetRenderTargets(1, //待绑定的RTV数量
		&rtvHandle, //指向RTV数组的指针
		true, //RTV对象在堆内存中是连续存放的
		&dsvHandle);//指向DSV的指针

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrcDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	////设置根签名
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->passCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(2, //根参数的起始索引
		passCB->GetGPUVirtualAddress());

	//绘制顶点（通过索引缓冲区绘制）
	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Opaque]);

	//cmdList->SetPipelineState(mPSOs["transparent"].Get());
	//DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Transparent]);

	//从渲染目标到呈现
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	//完成命令的记录关闭命令列表
	ThrowIfFailed(cmdList->Close());

	ID3D12CommandList* cmdLists[] = { cmdList.Get() };//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);//将命令从命令列表传至命令队列

	ThrowIfFailed(swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//FlushCmdQueue();
	mCurrFrameResource->fenceCPU = mCurrentFence++;
	cmdQueue->Signal(fence.Get(), mCurrentFence);
}

void StencilDemo::OnKeyboardInput() {
	const float dt = mTimer.DeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0X80000) {
		mSunTheta -= 1.0F * dt;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0X80000) {
		mSunTheta += 1.0F * dt;
	}
	if (GetAsyncKeyState(VK_UP) & 0X80000) {
		mSunPhi -= 1.0F * dt;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0X80000) {
		mSunPhi += 1.0F * dt;
	}

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1F, XM_PIDIV2);
}

void StencilDemo::UpdateCamera() {
	//构建观察矩阵
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);

	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, v);
}

void StencilDemo::UpdateObjectCBs() {
	auto currObjectCB = mCurrFrameResource->objCB.get();

	for (auto& e : mAllRenderItems)
	{
		if (e->NumFramesDirty > 0) {
			XMMATRIX world = XMLoadFloat4x4(&e->world);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.texTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->objCBIndex, objConstants);

			e->NumFramesDirty--;
		}

	}
}

void StencilDemo::UpdateMainPassCB() {
	XMMATRIX v = XMLoadFloat4x4(&mView);
	XMMATRIX p = XMLoadFloat4x4(&mProj);

	XMMATRIX VP_Matrix = v * p;

	PassConstants passConstants;
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

	passConstants.eyePosW = mEyePos;

	passConstants.totalTime = mTimer.TotalTime();

	/*passConstants.ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	passConstants.lights[0].direction = { 0.57735f, -0.57735f, 0.57735f };
	passConstants.lights[0].strength = { 0.6f, 0.6f, 0.6f };
	passConstants.lights[1].direction = { -0.57735f, -0.57735f, 0.57735f };
	passConstants.lights[1].strength = { 0.3f, 0.3f, 0.3f };
	passConstants.lights[2].direction = { 0.0f, -0.707f, -0.707f };
	passConstants.lights[2].strength = { 0.15f, 0.15f, 0.15f };*/

	passConstants.ambientLight = { 0.25f,0.25f,0.35f,1.0f };
	passConstants.lights[0].strength = { 1.0f,1.0f,0.9f };
	XMVECTOR sunDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	XMStoreFloat3(&passConstants.lights[0].direction, sunDir);

	passConstants.fogRange = 200.0f;
	passConstants.fogStart = 2.0f;

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(0, passConstants);
}

void StencilDemo::UpdateMatCBs() {
	auto currMaterialCB = mCurrFrameResource->matCB.get();

	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();//获得键值对的值，即Material指针（智能指针转普通指针）
		if (mat->numFramesDirty > 0) {
			//将定义的材质属性传给常量结构体中的元素

			MatConstants matConstants;
			matConstants.diffuseAlbedo = mat->diffuseAlbedo;
			matConstants.fresnelR0 = mat->fresnelR0;
			matConstants.roughness = mat->roughness;

			XMMATRIX matTransform = XMLoadFloat4x4(&mat->matTransform);
			XMStoreFloat4x4(&matConstants.matTransform, XMMatrixTranspose(matTransform));

			//将材质常量数据复制到常量缓冲区对应索引地址处
			currMaterialCB->CopyData(mat->matCBIndex, matConstants);
			//更新下一个帧资源
			mat->numFramesDirty--;
		}
	}
}

void StencilDemo::Update() {
	OnKeyboardInput();
	UpdateCamera();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % frameResourceCount;
	mCurrFrameResource = mFrameResource[mCurrFrameResourceIndex].get();
	//如果GPU端围栏值小于CPU端围栏值，即CPU速度快于GPU，则令CPU等待
	if (mCurrFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < mCurrFrameResource->fenceCPU) {
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->fenceCPU, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs();
	UpdateMatCBs();
	UpdateMainPassCB();
}

void StencilDemo::OnResize() {
	D3D12App::OnResize();

	//构建投影矩阵
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);
}

void StencilDemo::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void StencilDemo::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void StencilDemo::OnMouseMove(WPARAM btnState, int x, int y) {
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(static_cast<float>(x - mLastMousePos.x) * 0.25f);
		float dy = XMConvertToRadians(static_cast<float>(y - mLastMousePos.y) * 0.25f);

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, 3.1416f - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);
		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

array<const CD3DX12_STATIC_SAMPLER_DESC, 6> StencilDemo::GetStaticSamplers()
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
