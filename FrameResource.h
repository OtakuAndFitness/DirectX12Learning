#pragma once

#include "UploadBuffer.h"
#include "ProceduralGeometry.h"
#include "MathHelper.h"

//定义顶点结构体
struct Vertex
{
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

};

struct PassConstants {
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();

	XMFLOAT3 eyePosW = { 0.0f, 0.0f, 0.0f };
	float totalTime = 0.0f;
	XMFLOAT4 ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	Light lights[MaxLights];
};

struct MatConstants {
	XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	XMFLOAT3 fresnelR0 = { 0.01f,0.01f, 0.01f };
	float roughness = 0.25f;
	XMFLOAT4X4 matTransform = MathHelper::Identity4x4();
};

struct FrameResource
{
	public:
		FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT wavesVertCount);
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator = (const FrameResource& rhs) = delete;
		~FrameResource();
		//每帧资源都需要独立的命令分配器
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		//每帧都需要单独的资源缓冲区（此案例仅为2个常量缓冲区）
		unique_ptr<UploadBuffer<ObjectConstants>> objCB = nullptr;
		unique_ptr<UploadBuffer<PassConstants>> passCB = nullptr;
		unique_ptr<UploadBuffer<Vertex>> wavesVB = nullptr;
		unique_ptr<UploadBuffer<MatConstants>> matCB = nullptr;
		UINT64 fenceCPU = 0;

};