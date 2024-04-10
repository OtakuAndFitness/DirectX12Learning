#pragma once

#include "DxException.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "ProceduralGeometry.h"

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

struct FrameResource
{
	public:
		FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT wavesVertCount);
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator = (const FrameResource& rhs) = delete;
		~FrameResource();
		//每帧资源都需要独立的命令分配器
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		//每帧都需要单独的资源缓冲区（此案例仅为2个常量缓冲区）
		unique_ptr<UploadBuffer<ObjectConstants>> objCB = nullptr;
		unique_ptr<UploadBuffer<PassConstants>> passCB = nullptr;
		unique_ptr<UploadBuffer<Vertex>> wavesVB = nullptr;
		UINT64 fenceCPU = 0;

};