#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT wavesVertCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	objCB = make_unique<UploadBuffer<ObjectConstants>>(device, objCount, true);
	passCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);

	wavesVB = make_unique<UploadBuffer<Vertex>>(device, wavesVertCount, false);//一个顶点即一个子缓冲区

	matCB = make_unique<UploadBuffer<MatConstants>>(device, matCount, true);
}

FrameResource::~FrameResource()
{
}
