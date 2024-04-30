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
//class D3D12InitApp : public D3D12App {
//public:
//	D3D12InitApp(HINSTANCE hInstance);
//	D3D12InitApp(const D3D12InitApp& rhs) = delete;
//	D3D12InitApp& operator=(const D3D12InitApp& rhs) = delete;
//	~D3D12InitApp();
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
//	void UpdateObjectCBs();
//	void UpdateMaterialBuffer();
//	void UpdateMainPassCB();
//
//	void LoadTextures();
//	void BuildRootSignature();
//	void BuildDescriptorHeaps();
//	void BuildShadersAndInputLayout();
//	void BuildGeometry();
//	void BuildPSO();
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
//	ComPtr<ID3DBlob> mvsByteCode;
//	ComPtr<ID3DBlob> mpsByteCode;
//	ComPtr<ID3D12PipelineState> mPSO;
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
//
//	Camera mCamera;
//
//	unordered_map<string, unique_ptr<Material>> mMaterials;
//	unordered_map<string, unique_ptr<Texture>> mTextures;
//	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
//};
//
//D3D12InitApp::D3D12InitApp(HINSTANCE hInstance) : D3D12App(hInstance)
//{
//}
//
//D3D12InitApp::~D3D12InitApp()
//{
//	if (d3dDevice != nullptr) {
//		FlushCmdQueue();
//	}
//}
//
//void D3D12InitApp::LoadTextures()
//{
//
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
//	mTextures[bricksTex->name] = move(bricksTex);
//	mTextures[stoneTex->name] = move(stoneTex);
//	mTextures[tileTex->name] = move(tileTex);
//	mTextures[crateTex->name] = move(crateTex);
//}
//
//void D3D12InitApp::BuildGeometry() {
//	////实例化顶点结构体并填充
//	//array<Vertex, 8> vertices =
//	//{
//	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
//	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
//	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
//	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
//	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
//	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
//	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
//	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
//	//};
//
//	//array<uint16_t, 36> indices =
//	//{
//	//	// front face
//	//	0, 1, 2,
//	//	0, 2, 3,
//
//	//	// back face
//	//	4, 6, 5,
//	//	4, 7, 6,
//
//	//	// left face
//	//	4, 5, 1,
//	//	4, 1, 0,
//
//	//	// right face
//	//	3, 2, 6,
//	//	3, 6, 7,
//
//	//	// top face
//	//	1, 5, 6,
//	//	1, 6, 2,
//
//	//	// bottom face
//	//	4, 0, 3,
//	//	4, 3, 7
//	//};
//
//	//CONST UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//	//CONST UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);
//
//	//mGeo = make_unique<MeshGeometry>();
//
//	//ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeo->VertexBufferCPU));//创建顶点数据内存空间
//	//CopyMemory(mGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);//将顶点数据拷贝至顶点系统内存中
//
//	//ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeo->IndexBufferCPU));//创建索引数据内存空间
//	//CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);//将索引数据拷贝至索引系统内存中
//
//	//mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, mGeo->VertexBufferUploader);
//
//	//mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, mGeo->IndexBufferUploader);
//
//	//mGeo->VertexByteStride = sizeof(Vertex);
//	//mGeo->VertexBufferByteSize = vbByteSize;
//	//mGeo->IndexBufferByteSize = ibByteSize;
//
//	//SubmeshGeometry subMesh;
//	//subMesh.BaseVertexLocation = 0;
//	//subMesh.IndexCount = (UINT)indices.size();
//	//subMesh.StartIndexLocation = 0;
//	//mGeo->DrawArgs["box"] = subMesh;
//
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
//
//}
//
//void D3D12InitApp::BuildDescriptorHeaps() {
//	
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 4;
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
//}
//
////void D3D12InitApp::CreateConstantBufferViews()
////{
////	UINT objectCount = (UINT)mAllRenderItems.size();
////	UINT objConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
////	UINT passConstSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
////	//创建CBV堆
////	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
////	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
////	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
////	cbvHeapDesc.NumDescriptors = (objectCount + 1) * frameResourceCount;//此处一个堆中包含(几何体个数（包含实例）+1)个CBV
////	cbvHeapDesc.NodeMask = 0;
////	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
////	//elementCount为objectCount,22（22个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
////	mObjectCB = make_unique<UploadBuffer<ObjectConstants>>(d3dDevice.Get(), objectCount, true);
////	for (int frameIndex = 0; frameIndex < frameResourceCount; frameIndex++) {
////		//获得常量缓冲区首地址
////		for (int i = 0; i < objectCount; i++)
////		{
////			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress;
////			objCBAddress = mFrameResource[frameIndex]->objCB->Resource()->GetGPUVirtualAddress();
////			/*int objCbElementIndex = i;*/
////			objCBAddress += i * objConstSize;
////			int heapIndex = objectCount * frameIndex + i;
////			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
////			handle.Offset(heapIndex, csuDescriptorSize);
////			//创建CBV描述符
////			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
////			cbvDesc.BufferLocation = objCBAddress;
////			cbvDesc.SizeInBytes = objConstSize;
////			d3dDevice->CreateConstantBufferView(&cbvDesc, handle);
////		}
////	}
////	
////	mPassCB = make_unique<UploadBuffer<PassConstants>>(d3dDevice.Get(), 1, true);
////	for (int  frameIndex = 0; frameIndex < frameResourceCount; frameIndex++)
////	{
////		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress;
////		passCBAddress = mFrameResource[frameIndex]->passCB->Resource()->GetGPUVirtualAddress();
////		int passCbElementIndex = 0;
////		passCBAddress += passCbElementIndex * passConstSize;
////		int heapIndex = objectCount * frameResourceCount + frameIndex;
////		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
////		handle.Offset(heapIndex, csuDescriptorSize);
////		//创建CBV描述符
////		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
////		cbvDesc.BufferLocation = passCBAddress;
////		cbvDesc.SizeInBytes = passConstSize;
////		d3dDevice->CreateConstantBufferView(&cbvDesc, handle);
////	}
////	
////}
//
////void D3D12InitApp::BuildConstantBuffers() {
////	//elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
////	mObjectCB = make_unique<UploadBuffer<ObjectConstants>>(d3dDevice.Get(), 1, true);
////
////	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
////	
////	//获得常量缓冲区首地址
////	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
////	
////	//通过常量缓冲区元素偏移值计算最终的元素地址
////	int objCBufIndex = 0;//常量缓冲区元素下标
////	objCBAddress += objCBufIndex * objCBByteSize;
////	int heapIndex = 0;
////	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());//获得CBV堆首地址
////	handle.Offset(heapIndex, csuDescriptorSize);//CBV句柄（CBV堆中的CBV元素地址）
////	//创建CBV描述符
////	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
////	cbvDesc.BufferLocation = objCBAddress;
////	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
////
////	d3dDevice->CreateConstantBufferView(
////		&cbvDesc,
////		handle);
////
////	mPassCB = make_unique<UploadBuffer<PassConstants>>(d3dDevice.Get(), 1, true);
////	
////	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
////
////	//获得常量缓冲区首地址
////	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->Resource()->GetGPUVirtualAddress();
////
////	//通过常量缓冲区元素偏移值计算最终的元素地址
////	int passCBufIndex = 0;//常量缓冲区元素下标
////	passCBAddress += passCBufIndex * passCBByteSize;
////	heapIndex = 1;
////	handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());//获得CBV堆首地址
////	handle.Offset(heapIndex, csuDescriptorSize);//CBV句柄（CBV堆中的CBV元素地址）
////	//创建CBV描述符
////	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc1;
////	cbvDesc1.BufferLocation = passCBAddress;
////	cbvDesc1.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
////
////	d3dDevice->CreateConstantBufferView(
////		&cbvDesc1,
////		handle);
////}
//
//array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3D12InitApp::GetStaticSamplers()
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
//void D3D12InitApp::BuildRootSignature() {
//	//根参数可以是描述符表、根描述符、根常量
//	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
//	//创建由单个CBV所组成的描述符表
//	CD3DX12_DESCRIPTOR_RANGE texTable;
//	texTable.Init(
//		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,//描述符类型
//		4, //描述符数量
//		0);//描述符所绑定的寄存器槽号
//	
//	slotRootParameter[0].InitAsConstantBufferView(0);
//	slotRootParameter[1].InitAsConstantBufferView(1);
//	slotRootParameter[2].InitAsShaderResourceView(0, 1);
//	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
//
//	auto staticSamplers = GetStaticSamplers();
//
//	//根签名由一组根参数构成
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
//		4, //根参数的数量
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
//
//void D3D12InitApp::BuildPSO() {
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
//	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
//	opaqueDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
//	opaqueDesc.pRootSignature = mRootSignature.Get();
//	opaqueDesc.VS =
//	{
//		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
//		mvsByteCode->GetBufferSize()
//	};
//	opaqueDesc.PS =
//	{
//		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
//		mpsByteCode->GetBufferSize()
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
//	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSO)));
//}
//
//void D3D12InitApp::BuildShadersAndInputLayout() {
//
//	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
//	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");
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
//void D3D12InitApp::BuildMaterials()
//{
//	
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
//	mMaterials[bricks->name] = move(bricks);
//	mMaterials[stone->name] = move(stone);
//	mMaterials[tile->name] = move(tile);
//	mMaterials[crate->name] = move(crate);
//
//
//}
//
//void D3D12InitApp::BuildRenderItem()
//{
//	auto boxRenderItem = make_unique<RenderItem>();
//	XMStoreFloat4x4(&(boxRenderItem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
//	XMStoreFloat4x4(&boxRenderItem->texTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
//	boxRenderItem->objCBIndex = 0;//BOX常量数据（world矩阵）在objConstantBuffer索引0上
//	boxRenderItem->mat = mMaterials["crateMat"].get();
//	boxRenderItem->geo = mGeometries["shapeGeo"].get();
//	boxRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	boxRenderItem->indexCount = boxRenderItem->geo->DrawArgs["box"].IndexCount;
//	boxRenderItem->baseVertexLocation = boxRenderItem->geo->DrawArgs["box"].BaseVertexLocation;
//	boxRenderItem->startIndexLocation = boxRenderItem->geo->DrawArgs["box"].StartIndexLocation;
//	mAllRenderItems.push_back(move(boxRenderItem));
//
//	auto gridRenderItem = make_unique<RenderItem>();
//	gridRenderItem->world = MathHelper::Identity4x4();
//	XMStoreFloat4x4(&gridRenderItem->texTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
//	gridRenderItem->objCBIndex = 1;//grid常量数据（world矩阵）在objConstantBuffer索引1上
//	gridRenderItem->mat = mMaterials["tileMat"].get();
//	gridRenderItem->geo = mGeometries["shapeGeo"].get();
//	gridRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	gridRenderItem->indexCount = gridRenderItem->geo->DrawArgs["grid"].IndexCount;
//	gridRenderItem->baseVertexLocation = gridRenderItem->geo->DrawArgs["grid"].BaseVertexLocation;
//	gridRenderItem->startIndexLocation = gridRenderItem->geo->DrawArgs["grid"].StartIndexLocation;
//	mAllRenderItems.push_back(move(gridRenderItem));
//
//	XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
//	UINT followObjCBIndex = 2;//接下去的几何体常量数据在CB中的索引从2开始
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
//		leftSphereRenderItem->mat = mMaterials["stoneMat"].get();
//		leftSphereRenderItem->geo = mGeometries["shapeGeo"].get();
//		leftSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftSphereRenderItem->indexCount = leftSphereRenderItem->geo->DrawArgs["sphere"].IndexCount;
//		leftSphereRenderItem->baseVertexLocation = leftSphereRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
//		leftSphereRenderItem->startIndexLocation = leftSphereRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
//		//右边5个球
//		XMStoreFloat4x4(&(rightSphereRenderItem->world), rightSphereWorld);
//		rightSphereRenderItem->texTransform = MathHelper::Identity4x4();
//		rightSphereRenderItem->mat = mMaterials["stoneMat"].get();
//		rightSphereRenderItem->geo = mGeometries["shapeGeo"].get();
//		rightSphereRenderItem->objCBIndex = followObjCBIndex++;
//		rightSphereRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightSphereRenderItem->indexCount = rightSphereRenderItem->geo->DrawArgs["sphere"].IndexCount;
//		rightSphereRenderItem->baseVertexLocation = rightSphereRenderItem->geo->DrawArgs["sphere"].BaseVertexLocation;
//		rightSphereRenderItem->startIndexLocation = rightSphereRenderItem->geo->DrawArgs["sphere"].StartIndexLocation;
//
//		mAllRenderItems.push_back(move(leftCylinderRenderItem));
//		mAllRenderItems.push_back(move(rightCylinderRenderItem));
//		mAllRenderItems.push_back(move(leftSphereRenderItem));
//		mAllRenderItems.push_back(move(rightSphereRenderItem));
//
//	}
//
//	for (auto& e : mAllRenderItems)
//	{
//		renderItems.push_back(e.get());
//	}
//}
//
//bool D3D12InitApp::Init() {
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
//	//BuildConstantBuffers();
//	BuildRootSignature();
//	BuildShadersAndInputLayout();
//	BuildGeometry();
//	BuildMaterials();
//	BuildRenderItem();
//	BuildFrameResource();
//	//CreateConstantBufferViews();
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
//void D3D12InitApp::DrawRenderItems() {
//	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//	auto objectCB = mCurrFrameResource->objCB->Resource();
//
//	for (size_t i = 0; i < renderItems.size(); i++)
//	{
//		auto renderItem = renderItems[i];
//
//		cmdList->IASetVertexBuffers(0, 1, &renderItem->geo->GetVbv());
//		cmdList->IASetIndexBuffer(&renderItem->geo->GetIbv());
//		cmdList->IASetPrimitiveTopology(renderItem->primitiveType);
//
//		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + renderItem->objCBIndex * objCBByteSize;
//
//		//UINT objCbvIndex = mCurrFrameResourceIndex * (UINT)mAllRenderItems.size() + renderItem->objCBIndex;
//		//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//		//handle.Offset(objCbvIndex, csuDescriptorSize);
//		//cmdList->SetGraphicsRootDescriptorTable(0, handle);
//		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
//
//		cmdList->DrawIndexedInstanced(renderItem->indexCount, 1, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
//	}
//}
//
//void D3D12InitApp::BuildFrameResource()
//{
//	for (int i = 0; i < frameResourceCount; i++)
//	{
//		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
//			1, //passCount
//			(UINT)mAllRenderItems.size(),//objCount
//			(UINT)mMaterials.size()));
//	}
//}
//
//void D3D12InitApp::Draw() {
//	auto currCmdAllocator = mCurrFrameResource->cmdAllocator;
//	ThrowIfFailed(currCmdAllocator->Reset());//重复使用记录命令的相关内存
//	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSO.Get()));//复用命令列表及其内存
//
//	//设置视口和剪裁矩形
//	cmdList->RSSetViewports(1, &viewPort);
//	cmdList->RSSetScissorRects(1, &scissorRect);
//
//	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
//	//转换资源为后台缓冲区资源，从呈现到渲染目标转换
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
//	cmdList->ClearRenderTargetView(rtvHandle, Colors::LightBlue, 0, nullptr); //清除RT背景色为淡蓝，并且不设置裁剪矩形
//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
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
//	////设置CBV描述符堆
//	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
//	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//	////设置根签名
//	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
//	////设置顶点缓冲区
//	//cmdList->IASetVertexBuffers(0, 1, &mGeo->GetVbv());
//	////设置索引缓冲区
//	//cmdList->IASetIndexBuffer(&mGeo->GetIbv());
//	////将图元拓扑类型传入流水线
//	//cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//	////设置根描述符表
//	//int objCbvIndex = 0;
//	//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//	//handle.Offset(objCbvIndex, csuDescriptorSize);
//	//cmdList->SetGraphicsRootDescriptorTable(0, //根参数的起始索引
//	//	handle);
//
//	//int passCbvIndex = (int)mAllRenderItems.size() * frameResourceCount + mCurrFrameResourceIndex;
//	//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//	//handle.Offset(passCbvIndex, csuDescriptorSize);
//	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
//		//handle);
//
//	auto passCB = mCurrFrameResource->passCB->Resource();
//	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
//
//	auto matBuffer = mCurrFrameResource->materialBuffer->Resource();
//	cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
//
//	cmdList->SetGraphicsRootDescriptorTable(3, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
//
//	//绘制顶点（通过索引缓冲区绘制）
//	DrawRenderItems();
//	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["box"].IndexCount, //每个实例要绘制的索引数
//	//	1,//实例化个数
//	//	mGeo->DrawArgs["box"].StartIndexLocation,//起始索引位置
//	//	mGeo->DrawArgs["box"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
//	//	0);//实例化的高级技术，暂时设置为0
//
//	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
//	//	handle);
//	////绘制顶点（通过索引缓冲区绘制）
//	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["grid"].IndexCount, //每个实例要绘制的索引数
//	//	1,//实例化个数
//	//	mGeo->DrawArgs["grid"].StartIndexLocation,//起始索引位置
//	//	mGeo->DrawArgs["grid"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
//	//	0);//实例化的高级技术，暂时设置为0
//
//	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
//	//	handle);
//	////绘制顶点（通过索引缓冲区绘制）
//	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["sphere"].IndexCount, //每个实例要绘制的索引数
//	//	1,//实例化个数
//	//	mGeo->DrawArgs["sphere"].StartIndexLocation,//起始索引位置
//	//	mGeo->DrawArgs["sphere"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
//	//	0);//实例化的高级技术，暂时设置为0
//
//	//cmdList->SetGraphicsRootDescriptorTable(1, //根参数的起始索引
//	//	handle);
//	////绘制顶点（通过索引缓冲区绘制）
//	//cmdList->DrawIndexedInstanced(mGeo->DrawArgs["cylinder"].IndexCount, //每个实例要绘制的索引数
//	//	1,//实例化个数
//	//	mGeo->DrawArgs["cylinder"].StartIndexLocation,//起始索引位置
//	//	mGeo->DrawArgs["cylinder"].BaseVertexLocation,//子物体起始索引在全局索引中的位置
//	//	0);//实例化的高级技术，暂时设置为0
//
//	//从渲染目标到呈现
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//	//完成命令的记录关闭命令列表
//	ThrowIfFailed(cmdList->Close());
//
//	ID3D12CommandList* cmdLists[] = { cmdList.Get() };//声明并定义命令列表数组
//	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);//将命令从命令列表传至命令队列
//
//	ThrowIfFailed(swapChain->Present(0, 0));
//	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
//
//	//FlushCmdQueue();
//	mCurrFrameResource->fenceCPU = ++mCurrentFence;
//	cmdQueue->Signal(fence.Get(), mCurrentFence);
//}
//
//void D3D12InitApp::OnResize()
//{
//	D3D12App::OnResize();
//	//构建投影矩阵
//	//XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
//	//XMStoreFloat4x4(&mProj, p);
//	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//}
//
//void D3D12InitApp::OnKeyboardInput()
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
//void D3D12InitApp::UpdateObjectCBs()
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
//			currObjectCB->CopyData(e->objCBIndex, objConstants);
//
//			e->NumFramesDirty--;
//		}
//
//	}
//}
//
//void D3D12InitApp::UpdateMaterialBuffer()
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
//void D3D12InitApp::UpdateMainPassCB()
//{
//	XMMATRIX view = mCamera.GetView();
//	XMMATRIX proj = mCamera.GetProj();
//
//	XMMATRIX VP_Matrix =XMMatrixMultiply(view, proj);
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
////void D3D12InitApp::Update() {
////	ObjectConstants objConstants;
////	PassConstants passContants;
////	//构建观察矩阵
////	float x =mRadius * sinf(mPhi) * cosf(mTheta);
////	float y = mRadius * cosf(mPhi);
////	float z = mRadius * sinf(mPhi) * sinf(mTheta);
////	
////	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
////	XMVECTOR target = XMVectorZero();
////	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
////
////	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);
////
////	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % frameResourceCount;
////	mCurrFrameResource = mFrameResource[mCurrFrameResourceIndex].get();
////	//如果GPU端围栏值小于CPU端围栏值，即CPU速度快于GPU，则令CPU等待
////	if (mCurrFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < mCurrFrameResource->fenceCPU) {
////		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
////		ThrowIfFailed(fence->SetEventOnCompletion(mCurrFrameResource->fenceCPU, eventHandle));
////		WaitForSingleObject(eventHandle, INFINITE);
////		CloseHandle(eventHandle);
////	}
////
////	for (auto& e : mAllRenderItems)
////	{
////		if (e->NumFramesDirty > 0) {
////			mWorld = e->world;
////			XMMATRIX w = XMLoadFloat4x4(&mWorld);
////			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
////			mCurrFrameResource->objCB->CopyData(e->objCBIndex, objConstants);
////
////			e->NumFramesDirty--;
////		}
////		
////	}
////
////	XMMATRIX p = XMLoadFloat4x4(&mProj);
////	XMMATRIX VP_Matrix = v * p;
////	XMStoreFloat4x4(&passContants.viewProj, XMMatrixTranspose(VP_Matrix));
////	mCurrFrameResource->passCB->CopyData(0, passContants);
////	//XMStoreFloat4x4(&mView, v);
////	////构建世界矩阵
////	//XMMATRIX w = XMLoadFloat4x4(&mWorld);
////	////w *= XMMatrixTranslation(2.0f, 0.0f, 0.0f);
////
////	//XMMATRIX p = XMLoadFloat4x4(&mProj);
////	////矩阵计算
////	//XMMATRIX VP_Matrix = v * p;
////	////XMMATRIX赋值给XMFLOAT4X4
////	//XMStoreFloat4x4(&passContants.viewProj, XMMatrixTranspose(VP_Matrix));
////	////将数据拷贝至GPU缓存
////	//mPassCB->CopyData(0, passContants);
////
////	//XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
////	//mObjectCB->CopyData(0, objConstants);
//// 
////}
//
//void D3D12InitApp::Update() {
//	OnKeyboardInput();
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
//void D3D12InitApp::OnMouseDown(WPARAM btnState, int x, int y) {
//	mLastMousePos.x = x;
//	mLastMousePos.y = y;
//
//	SetCapture(mhMainWnd);
//}
//
//void D3D12InitApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//	ReleaseCapture();
//}
//
//void D3D12InitApp::OnMouseMove(WPARAM btnState, int x, int y)
//{
//	//if ((btnState & MK_LBUTTON) != 0) {
//	//	float dx = XMConvertToRadians(static_cast<float>(x - mLastMousePos.x) * 0.25f);
//	//	float dy = XMConvertToRadians(static_cast<float>(y - mLastMousePos.y) * 0.25f);
//	//	
//	//	mTheta += dx;
//	//	mPhi += dy;
//	//	
//	//	mPhi = MathHelper::Clamp(mPhi, 0.1f, 3.1416f - 0.1f);
//	//}
//	//else if ((btnState & MK_RBUTTON) != 0) {
//	//	float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
//	//	float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);
//	//	mRadius += dx - dy;
//	//	mRadius = MathHelper::Clamp(mRadius, 1.0f, 20.0f);
//	//}
//
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