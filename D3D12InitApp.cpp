#include "D3D12App.h"
#include "UploadBuffer.h"
#include "ProceduralGeometry.h"
//using namespace DirectX;

//定义顶点结构体
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

//单个物体的常量数据
struct ObjectConstants
{
	//初始化物体空间变换到裁剪空间矩阵，Identity4x4()是单位矩阵
	XMFLOAT4X4 world = MathHelper::Identity4x4();
};

struct PassConstants {
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
};

struct RenderItem {
	RenderItem() = default;

	//该几何体的世界矩阵
	XMFLOAT4X4 world = MathHelper::Identity4x4();
	
	//该几何体的常量数据在objConstantBuffer中的索引
	UINT objCBIndex = -1;
	
	//该几何体的图元拓扑类型
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	
	//该几何体的绘制三参数
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	UINT baseVertexLocation = 0;

};

class D3D12InitApp : public D3D12App {
public:
	D3D12InitApp(HINSTANCE hInstance);
	~D3D12InitApp();
	virtual bool Init(HINSTANCE hInstance, int nShowCmd)override;
	virtual void Update()override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	

private:
	void BuildGeometry();
	//void BuildDescriptorHeaps();
	//void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildPSO();
	void BuildShadersAndInputLayout();
	void BuildRenderItem();
	void CreateConstantBufferViews();
	void DrawRenderItems();
	virtual void Draw()override;
	virtual void OnResize()override;

private:
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	//定义并获得物体的常量缓冲区，然后得到其首地址
	unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	unique_ptr<UploadBuffer<PassConstants>> mPassCB = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;
	ComPtr<ID3D12PipelineState> mPSO = nullptr;
	unique_ptr<MeshGeometry> mGeo = nullptr;

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	POINT mLastMousePos;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	vector<unique_ptr<RenderItem>> mAllRenderItems;
	vector<RenderItem*> renderItems;
};

D3D12InitApp::D3D12InitApp(HINSTANCE hInstance) : D3D12App(hInstance)
{
}

D3D12InitApp::~D3D12InitApp()
{
}

void D3D12InitApp::BuildGeometry() {
	////实例化顶点结构体并填充
	//array<Vertex, 8> vertices =
	//{
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	//};

	//array<uint16_t, 36> indices =
	//{
	//	// front face
	//	0, 1, 2,
	//	0, 2, 3,

	//	// back face
	//	4, 6, 5,
	//	4, 7, 6,

	//	// left face
	//	4, 5, 1,
	//	4, 1, 0,

	//	// right face
	//	3, 2, 6,
	//	3, 6, 7,

	//	// top face
	//	1, 5, 6,
	//	1, 6, 2,

	//	// bottom face
	//	4, 0, 3,
	//	4, 3, 7
	//};

	//CONST UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	//CONST UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	//mGeo = make_unique<MeshGeometry>();

	//ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeo->VertexBufferCPU));//创建顶点数据内存空间
	//CopyMemory(mGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);//将顶点数据拷贝至顶点系统内存中

	//ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeo->IndexBufferCPU));//创建索引数据内存空间
	//CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);//将索引数据拷贝至索引系统内存中

	//mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, mGeo->VertexBufferUploader);

	//mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, mGeo->IndexBufferUploader);

	//mGeo->VertexByteStride = sizeof(Vertex);
	//mGeo->VertexBufferByteSize = vbByteSize;
	//mGeo->IndexBufferByteSize = ibByteSize;

	//SubmeshGeometry subMesh;
	//subMesh.BaseVertexLocation = 0;
	//subMesh.IndexCount = (UINT)indices.size();
	//subMesh.StartIndexLocation = 0;
	//mGeo->DrawArgs["box"] = subMesh;

	ProceduralGeometry geoMetry;
	ProceduralGeometry::MeshData box = geoMetry.CreateBox(1.5f, 0.5f, 1.5f, 3);
	ProceduralGeometry::MeshData grid = geoMetry.CreateGrid(20.0f, 30.0f, 60, 40);
	ProceduralGeometry::MeshData sphere = geoMetry.CreateSphere(0.5f, 20, 20);
	ProceduralGeometry::MeshData cylinder = geoMetry.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	
	//计算单个几何体顶点在总顶点数组中的偏移量,顺序为：box、grid、sphere、cylinder
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
	UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;

	//计算单个几何体索引在总索引数组中的偏移量,顺序为：box、grid、sphere、cylinder
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;

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

	auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
	vector<Vertex> vertices(totalVertexCount);//给定顶点数组大小

	int k = 0;
	for (int i = 0; i < box.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(Colors::Yellow);
	}

	for (int i = 0; i < grid.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(Colors::Brown);
	}

	for (int i = 0; i < sphere.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(Colors::Green);
	}

	for (int i = 0; i < cylinder.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(Colors::Blue);
	}

	vector<uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	mGeo = make_unique<MeshGeometry>();

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

}

//void D3D12InitApp::BuildDescriptorHeaps() {
//	
//	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
//	cbvHeapDesc.NumDescriptors = 2;
//	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	cbvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
//		IID_PPV_ARGS(&mCbvHeap)));
//
//}

void D3D12InitApp::CreateConstantBufferViews()
{
	UINT objectCount = (UINT)mAllRenderItems.size();
	UINT objConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT passConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	//创建CBV堆
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NumDescriptors = objectCount + 1;//此处一个堆中包含(几何体个数（包含实例）+1)个CBV
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
	//elementCount为objectCount,22（22个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
	mObjectCB = make_unique<UploadBuffer<ObjectConstants>>(d3dDevice.Get(), objectCount, true);
	//获得常量缓冲区首地址
	for (int i = 0; i < objectCount; i++)
	{
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress;
		objCBAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
		int objCbElementIndex = i;
		objCBAddress += objCbElementIndex * objConstSize;
		int heapIndex = i;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, csuDescriptorSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = objCBAddress;
		cbvDesc.SizeInBytes = objConstSize;
		d3dDevice->CreateConstantBufferView(&cbvDesc, handle);
	}

	mPassCB = make_unique<UploadBuffer<PassConstants>>(d3dDevice.Get(), 1, true);
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress;
	passCBAddress = mPassCB->Resource()->GetGPUVirtualAddress();
	int passCbElementIndex = 0;
	passCBAddress += passCbElementIndex * objConstSize;
	int heapIndex = objectCount;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(heapIndex, csuDescriptorSize);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = passCBAddress;
	cbvDesc.SizeInBytes = objConstSize;
	d3dDevice->CreateConstantBufferView(&cbvDesc, handle);

}

//void D3D12InitApp::BuildConstantBuffers() {
//	//elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
//	mObjectCB = make_unique<UploadBuffer<ObjectConstants>>(d3dDevice.Get(), 1, true);
//
//	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//	
//	//获得常量缓冲区首地址
//	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
//	
//	//通过常量缓冲区元素偏移值计算最终的元素地址
//	int objCBufIndex = 0;//常量缓冲区元素下标
//	objCBAddress += objCBufIndex * objCBByteSize;
//	int heapIndex = 0;
//	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());//获得CBV堆首地址
//	handle.Offset(heapIndex, csuDescriptorSize);//CBV句柄（CBV堆中的CBV元素地址）
//	//创建CBV描述符
//	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//	cbvDesc.BufferLocation = objCBAddress;
//	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//	d3dDevice->CreateConstantBufferView(
//		&cbvDesc,
//		handle);
//
//	mPassCB = make_unique<UploadBuffer<PassConstants>>(d3dDevice.Get(), 1, true);
//	
//	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
//
//	//获得常量缓冲区首地址
//	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->Resource()->GetGPUVirtualAddress();
//
//	//通过常量缓冲区元素偏移值计算最终的元素地址
//	int passCBufIndex = 0;//常量缓冲区元素下标
//	passCBAddress += passCBufIndex * passCBByteSize;
//	heapIndex = 1;
//	handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());//获得CBV堆首地址
//	handle.Offset(heapIndex, csuDescriptorSize);//CBV句柄（CBV堆中的CBV元素地址）
//	//创建CBV描述符
//	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc1;
//	cbvDesc1.BufferLocation = passCBAddress;
//	cbvDesc1.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
//
//	d3dDevice->CreateConstantBufferView(
//		&cbvDesc1,
//		handle);
//}

void D3D12InitApp::BuildRootSignature() {
	//根参数可以是描述符表、根描述符、根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	//创建由单个CBV所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,//描述符类型
		1, //描述符数量
		0);//描述符所绑定的寄存器槽号
	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,//描述符类型
		1, //描述符数量
		1);//描述符所绑定的寄存器槽号
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		2, //根参数的数量
		slotRootParameter, //根参数指针
		0, 
		nullptr,
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

void D3D12InitApp::BuildPSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void D3D12InitApp::BuildShadersAndInputLayout() {
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void D3D12InitApp::BuildRenderItem()
{
	auto boxRenderItem = make_unique<RenderItem>();
	XMStoreFloat4x4(&(boxRenderItem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRenderItem->objCBIndex = 0;//BOX常量数据（world矩阵）在objConstantBuffer索引0上
	boxRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRenderItem->indexCount = mGeo->DrawArgs["box"].IndexCount;
	boxRenderItem->baseVertexLocation = mGeo->DrawArgs["box"].BaseVertexLocation;
	boxRenderItem->startIndexLocation = mGeo->DrawArgs["box"].StartIndexLocation;
	mAllRenderItems.push_back(move(boxRenderItem));

	auto gridRenderItem = make_unique<RenderItem>();
	gridRenderItem->world = MathHelper::Identity4x4();
	gridRenderItem->objCBIndex = 1;//grid常量数据（world矩阵）在objConstantBuffer索引1上
	gridRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->indexCount = mGeo->DrawArgs["grid"].IndexCount;
	gridRenderItem->baseVertexLocation = mGeo->DrawArgs["grid"].BaseVertexLocation;
	gridRenderItem->startIndexLocation = mGeo->DrawArgs["grid"].StartIndexLocation;
	mAllRenderItems.push_back(move(gridRenderItem));

	UINT followObjCBIndex = 2;//接下去的几何体常量数据在CB中的索引从2开始
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
		//此处的索引随着循环不断加1（注意：这里是先赋值再++）
		leftCylinderRenderItem->objCBIndex = followObjCBIndex++;
		leftCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylinderRenderItem->indexCount = mGeo->DrawArgs["cylinder"].IndexCount;
		leftCylinderRenderItem->baseVertexLocation = mGeo->DrawArgs["cylinder"].BaseVertexLocation;
		leftCylinderRenderItem->startIndexLocation = mGeo->DrawArgs["cylinder"].StartIndexLocation;
		//右边5个圆柱
		XMStoreFloat4x4(&(rightCylinderRenderItem->world), rightCylinderWorld);
		rightCylinderRenderItem->objCBIndex = followObjCBIndex++;
		rightCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylinderRenderItem->indexCount = mGeo->DrawArgs["cylinder"].IndexCount;
		rightCylinderRenderItem->baseVertexLocation = mGeo->DrawArgs["cylinder"].BaseVertexLocation;
		rightCylinderRenderItem->startIndexLocation = mGeo->DrawArgs["cylinder"].StartIndexLocation;
		//左边5个球
		XMStoreFloat4x4(&(leftSphereRenderItem->world), leftSphereWorld);
		leftSphereRenderItem->objCBIndex = followObjCBIndex++;
		leftSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRenderItem->indexCount = mGeo->DrawArgs["sphere"].IndexCount;
		leftSphereRenderItem->baseVertexLocation = mGeo->DrawArgs["sphere"].BaseVertexLocation;
		leftSphereRenderItem->startIndexLocation = mGeo->DrawArgs["sphere"].StartIndexLocation;
		//右边5个球
		XMStoreFloat4x4(&(rightSphereRenderItem->world), rightSphereWorld);
		rightSphereRenderItem->objCBIndex = followObjCBIndex++;
		rightSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRenderItem->indexCount = mGeo->DrawArgs["sphere"].IndexCount;
		rightSphereRenderItem->baseVertexLocation = mGeo->DrawArgs["sphere"].BaseVertexLocation;
		rightSphereRenderItem->startIndexLocation = mGeo->DrawArgs["sphere"].StartIndexLocation;

		mAllRenderItems.push_back(move(leftCylinderRenderItem));
		mAllRenderItems.push_back(move(rightCylinderRenderItem));
		mAllRenderItems.push_back(move(leftSphereRenderItem));
		mAllRenderItems.push_back(move(rightSphereRenderItem));

	}

	for (auto& e : mAllRenderItems)
	{
		renderItems.push_back(e.get());
	}
}

bool D3D12InitApp::Init(HINSTANCE hInstance, int nShowCmd) {
	if (!D3D12App::Init(hInstance, nShowCmd)) {
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	//BuildDescriptorHeaps();
	//BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildRenderItem();
	CreateConstantBufferViews();
	BuildPSO();

	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void D3D12InitApp::DrawRenderItems() {
	for (size_t i = 0; i < renderItems.size(); i++)
	{
		auto renderItem = renderItems[i];

		cmdList->IASetVertexBuffers(0, 1, &mGeo->GetVbv());
		cmdList->IASetIndexBuffer(&mGeo->GetIbv());
		cmdList->IASetPrimitiveTopology(renderItem->primitiveType);

		UINT objCbvIndex = renderItem->objCBIndex;
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		handle.Offset(objCbvIndex, csuDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(0, handle);

		cmdList->DrawIndexedInstanced(renderItem->indexCount, 1, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
	}
}

void D3D12InitApp::Draw() {
	ThrowIfFailed(cmdAllocator->Reset());//重复使用记录命令的相关内存
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), mPSO.Get()));//复用命令列表及其内存

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
	
	////设置CBV描述符堆
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	////设置根签名
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
	////设置顶点缓冲区
	//cmdList->IASetVertexBuffers(0, 1, &mGeo->GetVbv());
	////设置索引缓冲区
	//cmdList->IASetIndexBuffer(&mGeo->GetIbv());
	////将图元拓扑类型传入流水线
	//cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	////设置根描述符表
	//int objCbvIndex = 0;
	//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//handle.Offset(objCbvIndex, csuDescriptorSize);
	//cmdList->SetGraphicsRootDescriptorTable(0, //根参数的起始索引
	//	handle);

	int passCbvIndex = (int)mAllRenderItems.size();
	auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	handle.Offset(passCbvIndex, csuDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
		handle);

	//绘制顶点（通过索引缓冲区绘制）
	DrawRenderItems();
	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["box"].IndexCount, //每个实例要绘制的索引数
	//	1,//实例化个数
	//	mGeo->DrawArgs["box"].StartIndexLocation,//起始索引位置
	//	mGeo->DrawArgs["box"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
	//	0);//实例化的高级技术，暂时设置为0

	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
	//	handle);
	////绘制顶点（通过索引缓冲区绘制）
	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["grid"].IndexCount, //每个实例要绘制的索引数
	//	1,//实例化个数
	//	mGeo->DrawArgs["grid"].StartIndexLocation,//起始索引位置
	//	mGeo->DrawArgs["grid"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
	//	0);//实例化的高级技术，暂时设置为0

	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
	//	handle);
	////绘制顶点（通过索引缓冲区绘制）
	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["sphere"].IndexCount, //每个实例要绘制的索引数
	//	1,//实例化个数
	//	mGeo->DrawArgs["sphere"].StartIndexLocation,//起始索引位置
	//	mGeo->DrawArgs["sphere"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
	//	0);//实例化的高级技术，暂时设置为0

	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
	//	handle);
	////绘制顶点（通过索引缓冲区绘制）
	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["cylinder"].IndexCount, //每个实例要绘制的索引数
	//	1,//实例化个数
	//	mGeo->DrawArgs["cylinder"].StartIndexLocation,//起始索引位置
	//	mGeo->DrawArgs["cylinder"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
	//	0);//实例化的高级技术，暂时设置为0

	//从渲染目标到呈现
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	//完成命令的记录关闭命令列表
	ThrowIfFailed(cmdList->Close());

	ID3D12CommandList* cmdLists[] = { cmdList.Get() };//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);//将命令从命令列表传至命令队列

	ThrowIfFailed(swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	FlushCmdQueue();
}

void D3D12InitApp::OnResize()
{
	D3D12App::OnResize();
	//构建投影矩阵
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);
}

void D3D12InitApp::Update() {
	ObjectConstants objConstants;
	PassConstants passContants;
	//构建观察矩阵
	float x =mRadius * sinf(mPhi) * cosf(mTheta);
	float y = mRadius * cosf(mPhi);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);
	for (auto& e : mAllRenderItems)
	{
		mWorld = e->world;
		XMMATRIX w = XMLoadFloat4x4(&mWorld);
		XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
		mObjectCB->CopyData(e->objCBIndex, objConstants);
	}

	XMMATRIX p = XMLoadFloat4x4(&mProj);
	XMMATRIX VP_Matrix = v * p;
	XMStoreFloat4x4(&passContants.viewProj, XMMatrixTranspose(VP_Matrix));
	mPassCB->CopyData(0, passContants);
	//XMStoreFloat4x4(&mView, v);
	////构建世界矩阵
	//XMMATRIX w = XMLoadFloat4x4(&mWorld);
	////w *= XMMatrixTranslation(2.0f, 0.0f, 0.0f);

	//XMMATRIX p = XMLoadFloat4x4(&mProj);
	////矩阵计算
	//XMMATRIX VP_Matrix = v * p;
	////XMMATRIX赋值给XMFLOAT4X4
	//XMStoreFloat4x4(&passContants.viewProj, XMMatrixTranspose(VP_Matrix));
	////将数据拷贝至GPU缓存
	//mPassCB->CopyData(0, passContants);

	//XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
	//mObjectCB->CopyData(0, objConstants);
}

void D3D12InitApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void D3D12InitApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3D12InitApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(static_cast<float>(x - mLastMousePos.x) * 0.25f);
		float dy = XMConvertToRadians(static_cast<float>(y - mLastMousePos.y) * 0.25f);
		
		mTheta += dx;
		mPhi += dy;
		
		mPhi = MathHelper::Clamp(mPhi, 0.1f, 3.1416f - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);
		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 1.0f, 20.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		D3D12InitApp theApp(hInstance);
		if (!theApp.Init(hInstance, nShowCmd)) {
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