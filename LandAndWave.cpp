#include "D3D12App.h"
#include "FrameResource.h"
#include "Waves.h"
#include "BlurFilter.h"

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
	AlphaTest,
	AlphaTestedTreeSprites,
	Count
};

class LandAndWave : public D3D12App {
public:
	LandAndWave(HINSTANCE hInstance);
	LandAndWave(const LandAndWave& rhs) = delete;
	LandAndWave& operator=(const LandAndWave& rhs) = delete;
	~LandAndWave();
	virtual bool Init()override;


private:
	void BuildDescriptorHeaps();
	void BuildLandGeometry();
	void BuildTreeBillboardGeometry();
	void BuildWaveGeometryBuffers();
	void BuildRootSignature();
	void BuildPostRootSignature();
	void BuildPSOs();
	void BuildShadersAndInputLayout();
	void BuildRenderItem();
	void DrawRenderItems(vector<RenderItem*>& items);
	void BuildFrameResource();
	void BuildMaterials();
	void BuildBoxGeometry();
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

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;
	array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
	void AnimateMaterials();
	void UpdateWaves();

private:
	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;

	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;
	
	unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCsuDescriptorHeap = nullptr;
	
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	POINT mLastMousePos;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	vector<unique_ptr<RenderItem>> mAllRenderItems;
	vector<RenderItem*> mRenderItemLayer[(int)RenderLayer::Count];
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;

	FrameResource* mCurrFrameResource = nullptr;
	vector<unique_ptr<FrameResource>> mFrameResource;
	int mCurrFrameResourceIndex = 0;

	//bool mIsWireframe = false;

	unique_ptr<Waves> mWaves;
	RenderItem* mWavesRenderItem = nullptr;

	unordered_map<string, unique_ptr<Material>> mMaterials;

	unordered_map<string, unique_ptr<Texture>> mTextures;

	unique_ptr<BlurFilter> mBlurFilter;

	ComPtr<ID3D12RootSignature> mPostProcessRootSignature = nullptr;
};

LandAndWave::LandAndWave(HINSTANCE hInstance) : D3D12App(hInstance)
{

}

LandAndWave::~LandAndWave()
{
	if (d3dDevice != nullptr) {
		FlushCmdQueue();
	}
}

bool LandAndWave::Init()
{
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	mWaves = make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	mBlurFilter = make_unique<BlurFilter>(d3dDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

	LoadTextures();
	BuildRootSignature();
	BuildPostRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildWaveGeometryBuffers();
	BuildBoxGeometry();
	BuildTreeBillboardGeometry();
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

void LandAndWave::BuildDescriptorHeaps()
{
	const int textureDescriptorCount = 4;
	const int burDescriptorCount = 4;

	//创建SRV堆
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = textureDescriptorCount + burDescriptorCount;
	//srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mCsuDescriptorHeap)));

	auto woodCrateTex = mTextures["box"]->resource;
	auto grassTex = mTextures["land"]->resource;
	auto lakeTex = mTextures["lake"]->resource;
	auto treeArrayTex = mTextures["treeArrayTex"]->resource;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mCsuDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	d3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, handle);

	handle.Offset(1, csuDescriptorSize);

	srvDesc.Format = lakeTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(lakeTex.Get(), &srvDesc, handle);

	handle.Offset(1, csuDescriptorSize);

	srvDesc.Format = woodCrateTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, handle);

	handle.Offset(1, csuDescriptorSize);

	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	d3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, handle);

	mBlurFilter->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mCsuDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 3, csuDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCsuDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 3, csuDescriptorSize),
		csuDescriptorSize);

}

void LandAndWave::BuildLandGeometry()
{
	//创建几何体，并将顶点和索引列表存储在MeshData中
	ProceduralGeometry proceGeo;
	ProceduralGeometry::MeshData grid = proceGeo.CreateGrid(160.0f, 160.0f, 50, 50);

	//封装grid的顶点、索引数据
	SubmeshGeometry gridSubMesh;
	gridSubMesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubMesh.BaseVertexLocation = 0;
	gridSubMesh.StartIndexLocation = 0;

	//创建顶点缓存
	size_t vertexCount = grid.Vertices.size();
	vector<Vertex> vertices(vertexCount);
	for (int i = 0; i < grid.Vertices.size(); i++)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;

		//根据顶点不同的y值，给予不同的顶点色(不同海拔对应的颜色)
		/*if (vertices[i].Pos.y < -10.0f) {
			vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if (vertices[i].Pos.y < 5.0f) {
			vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (vertices[i].Pos.y < 12.0f) {
			vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (vertices[i].Pos.y < 20.0f) {
			vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else {
			vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}*/
	}

	//创建索引缓存
	vector<uint16_t> indices = grid.GetIndices16();

	//计算顶点缓存和索引缓存大小
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "land";

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

	//将之前封装好的几何体的SubmeshGeometry对象赋值给无序映射表
	geo->DrawArgs["grid"] = gridSubMesh;
	//将“山川”的MeshGeometry装入总的几何体映射表
	mGeometries["land"] = move(geo);
}

void LandAndWave::BuildTreeBillboardGeometry()
{
	//定义树木公告板的顶点结构体
	struct TreeBillboardVertex {
		XMFLOAT3 Pos;//公告板的中心点坐标
		XMFLOAT2 Size;//公告板的长宽
	};

	const int treeCount = 16;

	array<TreeBillboardVertex, 16> vertices;
	for (UINT i = 0; i < treeCount; i++)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);//取-45到45的一个随机数
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);//获得y方向坐标（贴着地面）
		y += 8.0f;//让公告板中心点高于地面8.0个单位

		vertices[i].Pos = XMFLOAT3(x, y, z);//公告板的中心点坐标（世界坐标）
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);//公告板长宽

	}

	array<uint16_t, 16> indices;
	UINT j = 0;
	for (UINT i = 0; i < treeCount; i++, j++)
	{
		indices[j] = i;
	}
	
	const UINT vbByteSize = (UINT)treeCount * sizeof(TreeBillboardVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "treeBillboard";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));//创建顶点数据内存空间
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);//将顶点数据拷贝至顶点系统内存中

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));//创建索引数据内存空间
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);//将索引数据拷贝至索引系统内存中

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeBillboardVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry treeMesh;
	treeMesh.BaseVertexLocation = 0;
	treeMesh.StartIndexLocation = 0;
	treeMesh.IndexCount = (UINT)indices.size();

	geo->DrawArgs[geo->Name] = treeMesh;

	mGeometries[geo->Name] = move(geo);
}

float LandAndWave::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 LandAndWave::GetHillsNormal(float x, float z) const
{
	//通过偏微分方程求出法向量
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));//归一化法向量
	XMStoreFloat3(&n, unitNormal);//存为XMFLOAT3格式

	return n;
}

array<const CD3DX12_STATIC_SAMPLER_DESC, 6> LandAndWave::GetStaticSamplers()
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

void LandAndWave::AnimateMaterials()
{
	auto matLake = mMaterials["lake"].get();
	float& tu = matLake->matTransform(3, 0);
	float& tv = matLake->matTransform(3, 1);

	tu += 0.1f * mTimer.DeltaTime();
	tv += 0.02f * mTimer.DeltaTime();
	
	if (tu >= 1.0f) {
		tu -= 1.0f;
	}
	if (tv >= 1.0f) {
		tv -= 1.0f;
	}
	matLake->matTransform(3, 0) = tu;
	matLake->matTransform(3, 1) = tv;

	matLake->numFramesDirty = frameResourceCount;

}

void LandAndWave::BuildWaveGeometryBuffers()
{
	//初始化索引列表（每个三角形3个索引）
	vector<uint16_t> indices(3 * mWaves->TriangleCount());
	assert(mWaves->VertexCount() < 0x0000ffff);//顶点索引数大于65536则中止程序

	//填充索引列表
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m-1; i++)
	{
		for (int j = 0; j < n - 1; j++) {
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	//计算顶点和索引缓存大小
	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "lake";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	//创建索引的CPU系统内存
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	//将索引列表存入CPU系统内存
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	//将索引数据通过上传堆传至默认堆
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
	//赋值MeshGeomety中相关属性
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry lakeSubMesh;
	lakeSubMesh.IndexCount = (UINT)indices.size();
	lakeSubMesh.BaseVertexLocation = 0;
	lakeSubMesh.StartIndexLocation = 0;

	//使用waves几何体
	geo->DrawArgs["lake"] = lakeSubMesh;
	mGeometries["lake"] = move(geo);
}

void LandAndWave::BuildRootSignature()
{
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

//构建后处理签名
void LandAndWave::BuildPostRootSignature() {
	//创建SRV描述符表作为根参数0
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,//描述符类型SRV
		1,//描述符表数量
		0);//描述符所绑定的寄存器槽号

	//创建UAV描述符表作为根参数1
	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		1,
		0);

	//根参数可以是描述符表、根描述符、根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	slotRootParameter[0].InitAsConstants(12, 0);//12个常量，寄存器槽号为0
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);//Range数量为1
	slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);//Range数量为1

	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		3, //根参数的数量
		slotRootParameter, //根参数指针
		0, //静态采样器的数量0
		nullptr, //静态采样器指针为空
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
		IID_PPV_ARGS(&mPostProcessRootSignature)));


}

void LandAndWave::BuildPSOs()
{
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

	//D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframeDesc = opaqueDesc;
	//wireframeDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&wireframeDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
	
	//AlphaTest物体的PSO（不需要混合）
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestDesc = opaqueDesc;
	alphaTestDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["alphaTestPS"]->GetBufferPointer()),
		mShaders["alphaTestPS"]->GetBufferSize()
	};
	alphaTestDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//双面显示
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&alphaTestDesc, IID_PPV_ARGS(&mPSOs["alphaTest"])));

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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeBillboardPsoDesc = opaqueDesc;
	treeBillboardPsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeBillboardPsoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["treeBillboardVS"]->GetBufferPointer()),
		mShaders["treeBillboardVS"]->GetBufferSize()
	};
	treeBillboardPsoDesc.GS = {
		reinterpret_cast<BYTE*>(mShaders["treeBillboardGS"]->GetBufferPointer()),
		mShaders["treeBillboardGS"]->GetBufferSize()
	};
	treeBillboardPsoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["treeBillboardPS"]->GetBufferPointer()),
		mShaders["treeBillboardPS"]->GetBufferSize()
	};
	treeBillboardPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeBillboardPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&treeBillboardPsoDesc, IID_PPV_ARGS(&mPSOs["treeBillboard"])));

	D3D12_COMPUTE_PIPELINE_STATE_DESC horizontalBlurPso = {  };
	horizontalBlurPso.pRootSignature = mPostProcessRootSignature.Get();
	horizontalBlurPso.CS = {
		reinterpret_cast<BYTE*>(mShaders["horizontalBlurCS"]->GetBufferPointer()),
		mShaders["horizontalBlurCS"]->GetBufferSize()
	};
	horizontalBlurPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&horizontalBlurPso, IID_PPV_ARGS(&mPSOs["horizontalBlur"])))

	D3D12_COMPUTE_PIPELINE_STATE_DESC verticalBlurPso = {  };
	verticalBlurPso.pRootSignature = mPostProcessRootSignature.Get();
	verticalBlurPso.CS = {
		reinterpret_cast<BYTE*>(mShaders["verticalBlurCS"]->GetBufferPointer()),
		mShaders["verticalBlurCS"]->GetBufferSize()
	};
	verticalBlurPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&verticalBlurPso, IID_PPV_ARGS(&mPSOs["verticalBlur"])))
}

void LandAndWave::BuildShadersAndInputLayout()
{
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

	mShaders["treeBillboardVS"] = d3dUtil::CompileShader(L"Shaders\\TreeBillboard.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["treeBillboardGS"] = d3dUtil::CompileShader(L"Shaders\\TreeBillboard.hlsl", nullptr, "GS", "gs_5_0");
	mShaders["treeBillboardPS"] = d3dUtil::CompileShader(L"Shaders\\TreeBillboard.hlsl", alphaTestDefines, "PS", "ps_5_0");

	mShaders["horizontalBlurCS"] = d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "HorizontalBlurCS", "cs_5_0");
	mShaders["verticalBlurCS"] = d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "VerticalBlurCS", "cs_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};

	mTreeSpriteInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
}

void LandAndWave::BuildRenderItem()
{
	auto gridItem = make_unique<RenderItem>();
	gridItem->world = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridItem->texTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridItem->objCBIndex = 0;
	gridItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridItem->geo = mGeometries["land"].get();
	gridItem->mat = mMaterials["land"].get();
	gridItem->indexCount = gridItem->geo->DrawArgs["grid"].IndexCount;
	gridItem->baseVertexLocation = gridItem->geo->DrawArgs["grid"].BaseVertexLocation;
	gridItem->startIndexLocation = gridItem->geo->DrawArgs["grid"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(gridItem.get());

	auto wavesItem = make_unique<RenderItem>();
	wavesItem->world = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesItem->texTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesItem->objCBIndex = 1;
	wavesItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesItem->geo = mGeometries["lake"].get();
	wavesItem->mat = mMaterials["lake"].get();
	wavesItem->indexCount = wavesItem->geo->DrawArgs["lake"].IndexCount;
	wavesItem->baseVertexLocation = wavesItem->geo->DrawArgs["lake"].BaseVertexLocation;
	wavesItem->startIndexLocation = wavesItem->geo->DrawArgs["lake"].StartIndexLocation;
	mWavesRenderItem = wavesItem.get();
	mRenderItemLayer[(int)RenderLayer::Transparent].push_back(wavesItem.get());

	auto boxItem = make_unique<RenderItem>();
	XMStoreFloat4x4(&boxItem->world, XMMatrixTranslation(3.0f, 2.0f, -9.0f));
	boxItem->objCBIndex = 2;
	boxItem->mat = mMaterials["box"].get();
	boxItem->geo = mGeometries["box"].get();
	boxItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxItem->indexCount = boxItem->geo->DrawArgs["box"].IndexCount;
	boxItem->baseVertexLocation = boxItem->geo->DrawArgs["box"].BaseVertexLocation;
	boxItem->startIndexLocation = boxItem->geo->DrawArgs["box"].StartIndexLocation;
	boxItem->texTransform= MathHelper::Identity4x4();
	mRenderItemLayer[(int)RenderLayer::AlphaTest].push_back(boxItem.get());

	auto treeBillboardItem = make_unique<RenderItem>();
	treeBillboardItem->world = MathHelper::Identity4x4();
	treeBillboardItem->texTransform = MathHelper::Identity4x4();
	treeBillboardItem->objCBIndex = 3;
	treeBillboardItem->mat = mMaterials["treeBillboardMat"].get();
	treeBillboardItem->geo = mGeometries["treeBillboard"].get();
	treeBillboardItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;//几何着色器只接受点列表和线列表
	treeBillboardItem->indexCount = treeBillboardItem->geo->DrawArgs["treeBillboard"].IndexCount;
	treeBillboardItem->baseVertexLocation = treeBillboardItem->geo->DrawArgs["treeBillboard"].BaseVertexLocation;
	treeBillboardItem->startIndexLocation = treeBillboardItem->geo->DrawArgs["treeBillboard"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeBillboardItem.get());

	mAllRenderItems.push_back(move(wavesItem));
	mAllRenderItems.push_back(move(gridItem));
	mAllRenderItems.push_back(move(boxItem));
	mAllRenderItems.push_back(move(treeBillboardItem));
}

void LandAndWave::DrawRenderItems(vector<RenderItem*>& items)
{
	//UINT objectCount = (UINT)mAllRenderItems.size();//物体总个数（包括实例）
	UINT objConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//UINT passConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	UINT matConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MatConstants));

	auto objCB = mCurrFrameResource->objCB->Resource();
	auto matCB = mCurrFrameResource->matCB->Resource();


	for (size_t i = 0; i < items.size(); i++)
	{
		auto ri = items[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->geo->GetVbv());
		cmdList->IASetIndexBuffer(&ri->geo->GetIbv());
		cmdList->IASetPrimitiveTopology(ri->primitiveType);
		
		CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(mCsuDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
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

void LandAndWave::BuildFrameResource()
{
	for (int i = 0; i < frameResourceCount; i++)
	{
		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
			1, //passCount
			(UINT)mAllRenderItems.size(),//objCount
			(UINT)mMaterials.size(),
			mWaves->VertexCount()));
	}
}

void LandAndWave::BuildMaterials()
{
	//定义陆地的材质
	auto land = make_unique<Material>();//Material指针
	land->name = "land";
	land->matCBIndex = 0;
	land->diffuseSrvHeapIndex = 0;
	land->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);//陆地的反照率（颜色）
	land->fresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);//陆地的R0
	land->roughness = 0.125f;//陆地的粗糙度（归一化后的）

	//定义湖水的材质
	auto lake = make_unique<Material>();
	lake->name = "lake";
	lake->matCBIndex = 1;
	lake->diffuseSrvHeapIndex = 1;
	lake->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);//湖水的反照率（颜色）
	lake->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);//湖水的R0（因为没有透明度和折射率，所以这里给0.1）
	lake->roughness = 0.0f;//湖水的粗糙度（归一化后的）

	auto box = make_unique<Material>();
	box->name = "wood";
	box->matCBIndex = 2;
	box->diffuseSrvHeapIndex = 2;
	box->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	box->roughness = 0.25f;

	auto treeBillboard = make_unique<Material>();
	treeBillboard->name = "treeBillboardMat";
	treeBillboard->matCBIndex = 3;
	treeBillboard->diffuseSrvHeapIndex = 3;
	treeBillboard->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeBillboard->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	treeBillboard->roughness = 0.8f;

	mMaterials["land"] = move(land);
	mMaterials["lake"] = move(lake);
	mMaterials["box"] = move(box);
	mMaterials[treeBillboard->name] = move(treeBillboard);
}

void LandAndWave::BuildBoxGeometry()
{
	ProceduralGeometry boxGeo;
	ProceduralGeometry::MeshData box = boxGeo.CreateBox(8.0f, 8.0f, 8.0f, 3);

	size_t verticesCount = box.Vertices.size();
	vector<Vertex> vertices(verticesCount);
	for (size_t i = 0; i < verticesCount; i++)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	vector<uint16_t> indices = box.GetIndices16();

	const UINT vbByteSize = (UINT)verticesCount * sizeof(Vertex);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.IndexCount = (UINT)indices.size();

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "Box";
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->DrawArgs["box"] = submesh;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	mGeometries["box"] = move(geo);
}

void LandAndWave::LoadTextures()
{
	//板条箱纹理
	auto woodCrateTex = make_unique<Texture>();
	woodCrateTex->name = "fenceTex";
	woodCrateTex->fileName = L"Textures/WireFence.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), woodCrateTex->fileName.c_str(), woodCrateTex->resource, woodCrateTex->uploadHeap));

	//草地纹理
	auto grassTex = make_unique<Texture>();
	grassTex->name = "grassTex";
	grassTex->fileName = L"Textures/grass.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), grassTex->fileName.c_str(), grassTex->resource, grassTex->uploadHeap));

	//湖水纹理
	auto lakeTex = make_unique<Texture>();
	lakeTex->name = "lakeTex";
	lakeTex->fileName = L"Textures/water1.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), lakeTex->fileName.c_str(), lakeTex->resource, lakeTex->uploadHeap));

	auto treeArrayTex = make_unique<Texture>();
	treeArrayTex->name = "treeArrayTex";
	treeArrayTex->fileName = L"Textures/treeArray2.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(), treeArrayTex->fileName.c_str(), treeArrayTex->resource, treeArrayTex->uploadHeap));

	mTextures["box"] = move(woodCrateTex);
	mTextures["land"] = move(grassTex);
	mTextures["lake"] = move(lakeTex);
	mTextures[treeArrayTex->name] = move(treeArrayTex);
}

void LandAndWave::UpdateWaves() {
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.25f) {
		t_base += 0.25f;//0.25秒生成一个波浪
		//随机生成横坐标
		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		//随机生成纵坐标
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);
		//随机生成波的半径
		float r = MathHelper::RandF(0.2f, 0.5f);
		//使用波动方程函数生成波纹
		mWaves->Disturb(i, j, r);
	}

	//每帧更新波浪模拟（即更新顶点坐标）
	mWaves->Update(mTimer.DeltaTime());

	//将更新的顶点坐标存入GPU上传堆中
	auto currWavesVB = mCurrFrameResource->wavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); i++)
	{
		Vertex v;
		v.Pos = mWaves->Position(i);
		//v.Color = XMFLOAT4(Colors::Blue);
		v.Normal = mWaves->Normal(i);
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}
	//赋值湖泊的GPU上的顶点缓存
	mWavesRenderItem->geo->VertexBufferGPU = currWavesVB->Resource();
}

void LandAndWave::Draw()
{
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

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCsuDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	////设置根签名
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->passCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(2, //根参数的起始索引
		passCB->GetGPUVirtualAddress());

	//绘制顶点（通过索引缓冲区绘制）
	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Opaque]);
	cmdList->SetPipelineState(mPSOs["alphaTest"].Get());
	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::AlphaTest]);
	cmdList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Transparent]);
	cmdList->SetPipelineState(mPSOs["treeBillboard"].Get());
	DrawRenderItems(mRenderItemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);
	
	//执行离屏模糊计算，得到模糊后的离屏纹理blurMap0
	mBlurFilter->Execute(cmdList.Get(), mPostProcessRootSignature.Get(), mPSOs["horizontalBlur"].Get(), mPSOs["verticalBlur"].Get(), swapChainBuffer[ref_mCurrentBackBuffer].Get(), 4);
	//将后台缓冲资源转成“复制目标”
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	//将模糊处理后的离屏纹理拷贝给后台缓冲区
	cmdList->CopyResource(swapChainBuffer[ref_mCurrentBackBuffer].Get(), mBlurFilter->Output());
	//再次转换RT资源，转成呈现
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));


	////从渲染目标到呈现
	//cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	//完成命令的记录关闭命令列表
	ThrowIfFailed(cmdList->Close());

	ID3D12CommandList* cmdLists[] = { cmdList.Get() };//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);//将命令从命令列表传至命令队列

	ThrowIfFailed(swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//FlushCmdQueue();
	mCurrFrameResource->fenceCPU = ++mCurrentFence;
	cmdQueue->Signal(fence.Get(), mCurrentFence);
}

void LandAndWave::OnKeyboardInput() {
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

	/*if (GetAsyncKeyState('1') & 0x8000) {
		mIsWireframe = true;
	}
	else {
		mIsWireframe = false;
	}*/
}

void LandAndWave::UpdateCamera() {
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

void LandAndWave::UpdateObjectCBs() {
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

void LandAndWave::UpdateMainPassCB() {
	XMMATRIX v = XMLoadFloat4x4(&mView);
	XMMATRIX p = XMLoadFloat4x4(&mProj);
	
	XMMATRIX VP_Matrix = v * p;

	PassConstants passConstants;
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

	passConstants.eyePosW = mEyePos;

	passConstants.ambientLight = { 0.25f,0.25f,0.35f,1.0f };
	passConstants.lights[0].strength = { 1.0f,1.0f,0.9f };
	passConstants.totalTime = mTimer.TotalTime();
	XMVECTOR sunDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	XMStoreFloat3(&passConstants.lights[0].direction, sunDir);

	passConstants.fogRange = 200.0f;
	passConstants.fogStart = 2.0f;

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(0, passConstants);
}

void LandAndWave::UpdateMatCBs()
{
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

void LandAndWave::OnResize()
{
	D3D12App::OnResize();

	//构建投影矩阵
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);

	if (mBlurFilter != nullptr) {
		mBlurFilter->OnResize((UINT)mClientWidth, (UINT)mClientHeight);
	}
}

void LandAndWave::Update()
{
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

	AnimateMaterials();
	UpdateObjectCBs();
	UpdateMatCBs();
	UpdateMainPassCB();
	UpdateWaves();

}

void LandAndWave::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void LandAndWave::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LandAndWave::OnMouseMove(WPARAM btnState, int x, int y)
{
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