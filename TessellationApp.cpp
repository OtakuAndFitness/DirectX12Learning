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
	Count
};

class Tessellation : public D3D12App {
public:
	Tessellation(HINSTANCE hInstance);
	Tessellation(const Tessellation& rhs) = delete;
	Tessellation& operator=(const Tessellation& rhs) = delete;
	~Tessellation();
	virtual bool Init()override;


private:
	void BuildDescriptorHeaps();
	void BuildRootSignature();
	void BuildPSOs();
	void BuildShadersAndInputLayout();
	void BuildQuadPatchGeometry();
	void BuildRenderItem();
	void DrawRenderItems(vector<RenderItem*>& items);
	void BuildFrameResource();
	void BuildMaterials();
	void LoadTextures();

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

	vector<unique_ptr<RenderItem>> mAllRenderItems;
	vector<RenderItem*> mRenderItemLayer[(int)RenderLayer::Count];
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;

	FrameResource* mCurrFrameResource = nullptr;
	vector<unique_ptr<FrameResource>> mFrameResource;
	int mCurrFrameResourceIndex = 0;

	//bool mIsWireframe = false;

	RenderItem* mWavesRenderItem = nullptr;

	unordered_map<string, unique_ptr<Material>> mMaterials;

	unordered_map<string, unique_ptr<Texture>> mTextures;

	ComPtr<ID3D12RootSignature> mPostProcessRootSignature = nullptr;
};

Tessellation::Tessellation(HINSTANCE hInstance) : D3D12App(hInstance)
{

}

Tessellation::~Tessellation()
{
	if (d3dDevice != nullptr) {
		FlushCmdQueue();
	}
}

bool Tessellation::Init()
{
	if (!D3D12App::Init()) {
		return false;
	}

	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildQuadPatchGeometry();
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

void Tessellation::BuildDescriptorHeaps()
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

}



array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Tessellation::GetStaticSamplers()
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


void Tessellation::BuildRootSignature()
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

void Tessellation::BuildPSOs()
{
	//不透明物体的PSO（不需要混合）
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaqueDesc.pRootSignature = mRootSignature.Get();
	opaqueDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessVS"]->GetBufferPointer()),
		mShaders["tessVS"]->GetBufferSize()
	};
	opaqueDesc.HS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessHS"]->GetBufferPointer()),
		mShaders["tessHS"]->GetBufferSize()
	};
	opaqueDesc.DS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessDS"]->GetBufferPointer()),
		mShaders["tessDS"]->GetBufferSize()
	};
	opaqueDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessPS"]->GetBufferPointer()),
		mShaders["tessPS"]->GetBufferSize()
	};
	opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaqueDesc.SampleMask = UINT_MAX;
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	opaqueDesc.SampleDesc.Count = 1;
	opaqueDesc.SampleDesc.Quality = 0;
	opaqueDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
}

void Tessellation::BuildShadersAndInputLayout()
{

	mShaders["tessVS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["tessHS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["tessDS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["tessPS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "PS", "ps_5_0");
	
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

	};
}

void Tessellation::BuildQuadPatchGeometry()
{
	vector<Vertex> vertices(4);
	vertices[0].Pos = { -10.0f,0.0f,10.0f };
	vertices[1].Pos = { 10.0f,0.0f,10.0f };
	vertices[2].Pos = { -10.0f,0.0f,-10.0f };
	vertices[3].Pos = { 10.0f,0.0f,-10.0f };

	vector<int16_t> indices = { 0,1,2,3 };

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(int16_t);

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.IndexCount = (UINT)indices.size();

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "quadPatchGeo";

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->DrawArgs[geo->Name] = submesh;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	mGeometries[geo->Name] = move(geo);

}

void Tessellation::BuildRenderItem()
{
	auto gridItem = make_unique<RenderItem>();
	gridItem->world = MathHelper::Identity4x4();
	gridItem->objCBIndex = 0;
	gridItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;//4个控制点的patch列表
	gridItem->geo = mGeometries["quadPatchGeo"].get();
	gridItem->mat = mMaterials["quadPatchGeo"].get();
	gridItem->indexCount = gridItem->geo->DrawArgs["quadPatchGeo"].IndexCount;
	gridItem->baseVertexLocation = gridItem->geo->DrawArgs["quadPatchGeo"].BaseVertexLocation;
	gridItem->startIndexLocation = gridItem->geo->DrawArgs["quadPatchGeo"].StartIndexLocation;
	mRenderItemLayer[(int)RenderLayer::Opaque].push_back(gridItem.get());

	mAllRenderItems.push_back(move(gridItem));
}

void Tessellation::DrawRenderItems(vector<RenderItem*>& items)
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

void Tessellation::BuildFrameResource()
{
	for (int i = 0; i < frameResourceCount; i++)
	{
		mFrameResource.push_back(make_unique<FrameResource>(d3dDevice.Get(),
			1, //passCount
			(UINT)mAllRenderItems.size(),//objCount
			(UINT)mMaterials.size()));
	}
}

void Tessellation::BuildMaterials()
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

void Tessellation::LoadTextures()
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

void Tessellation::Draw()
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

	////从渲染目标到呈现
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

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

void Tessellation::UpdateCamera() {
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

void Tessellation::UpdateObjectCBs() {
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

void Tessellation::UpdateMainPassCB() {
	XMMATRIX v = XMLoadFloat4x4(&mView);
	XMMATRIX p = XMLoadFloat4x4(&mProj);

	XMMATRIX VP_Matrix = v * p;

	PassConstants passConstants;
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

	passConstants.eyePosW = mEyePos;

	passConstants.ambientLight = { 0.25f,0.25f,0.35f,1.0f };
	passConstants.lights[0].strength = { 1.0f,1.0f,0.9f };
	passConstants.totalTime = mTimer.TotalTime();

	passConstants.fogRange = 200.0f;
	passConstants.fogStart = 2.0f;

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(0, passConstants);
}

void Tessellation::UpdateMatCBs()
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

void Tessellation::OnResize()
{
	D3D12App::OnResize();

	//构建投影矩阵
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);

}

void Tessellation::Update()
{
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

void Tessellation::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Tessellation::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Tessellation::OnMouseMove(WPARAM btnState, int x, int y)
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd) {
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Tessellation theApp(hInstance);
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