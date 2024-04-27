#pragma once

#include "UploadBuffer.h"
#include "ProceduralGeometry.h"
#include "MathHelper.h"

//定义顶点结构体
struct Vertex
{
	Vertex() = default;
	Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) : Pos(x,z,y), Normal(nx,ny,nz), TexC(u,v){}

	XMFLOAT3 Pos;
	//XMFLOAT4 Color;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;
};

//单个物体的常量数据
struct ObjectConstants
{
	//初始化物体空间变换到裁剪空间矩阵，Identity4x4()是单位矩阵
	XMFLOAT4X4 world = MathHelper::Identity4x4();
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4();

	UINT materialIndex = 0;
	UINT objPad0;
	UINT objPad1;
	UINT objPad2;

};

struct PassConstants {
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();

	XMFLOAT3 eyePosW = { 0.0f, 0.0f, 0.0f };
	float totalTime = 0.0f;
	XMFLOAT4 ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	Light lights[MaxLights];

	XMFLOAT4 fogColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float fogStart = 1.0f;//雾的起始距离
	float fogRange = 1.0f;//雾的衰减范围
	XMFLOAT2 pad2 = { 0.0f, 0.0f };//占位
};

//struct MatConstants {
//	XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
//	XMFLOAT3 fresnelR0 = { 0.01f,0.01f, 0.01f };
//	float roughness = 0.25f;
//	XMFLOAT4X4 matTransform = MathHelper::Identity4x4();
//};

struct MaterialData {
	XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	XMFLOAT3 fresnelR0 = { 0.01f,0.01f, 0.01f };
	float roughness = 0.25f;
	XMFLOAT4X4 matTransform = MathHelper::Identity4x4();

	UINT diffuseMapIndex = 0;
	UINT matPad0;
	UINT matPad1;
	UINT matPad2;

};

struct FrameResource
{
	public:
		FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT wavesVertCount);
		FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount);
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator = (const FrameResource& rhs) = delete;
		~FrameResource();
		//每帧资源都需要独立的命令分配器
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		//每帧都需要单独的资源缓冲区（此案例仅为2个常量缓冲区）
		unique_ptr<UploadBuffer<ObjectConstants>> objCB = nullptr;
		unique_ptr<UploadBuffer<PassConstants>> passCB = nullptr;
		unique_ptr<UploadBuffer<Vertex>> wavesVB = nullptr;
		unique_ptr<UploadBuffer<MaterialData>> matSB = nullptr;
		UINT64 fenceCPU = 0;

};