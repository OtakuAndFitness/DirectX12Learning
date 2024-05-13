#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT wavesVertCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	objCB = make_unique<UploadBuffer<ObjectConstants>>(device, objCount, true);
	passCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);

	wavesVB = make_unique<UploadBuffer<Vertex>>(device, wavesVertCount, false);//一个顶点即一个子缓冲区

	materialBuffer = make_unique<UploadBuffer<MaterialData>>(device, matCount, false);
}

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	objCB = make_unique<UploadBuffer<ObjectConstants>>(device, objCount, true);
	passCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	materialBuffer = make_unique<UploadBuffer<MaterialData>>(device, matCount, false);
	ssaoCB = make_unique<UploadBuffer<SsaoConstants>>(device, 1, true);
}

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT skinnedObjectCount, UINT matCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	objCB = make_unique<UploadBuffer<ObjectConstants>>(device, objCount, true);
	passCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	materialBuffer = make_unique<UploadBuffer<MaterialData>>(device, matCount, false);
	ssaoCB = make_unique<UploadBuffer<SsaoConstants>>(device, 1, true);
	skinnedCB = make_unique<UploadBuffer<SkinnedConstants>>(device, skinnedObjectCount, true);
}

//FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT matCount)
//{
//	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
//
//	instanceBuffer = make_unique<UploadBuffer<InstanceData>>(device, maxInstanceCount, false);
//	passCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
//
//	materialBuffer = make_unique<UploadBuffer<MaterialData>>(device, matCount, false);
//}

FrameResource::~FrameResource()
{
}
