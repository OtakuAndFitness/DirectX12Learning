#include "SSAO.h"

using namespace DirectX::PackedVector;

SSAO::SSAO(ID3D12Device* device, ID3D12GraphicsCommandList* list, UINT width, UINT height)
{
	mDevice = device;

	OnResize(width, height);

	BuildOffsetVectors();
	BuildRandomVectorTexture(list);
}

UINT SSAO::Width() const
{
	return mWidth / 2;
}

UINT SSAO::Height() const
{
	return mHeight / 2;
}

void SSAO::GetOffsetVectors(XMFLOAT4 offsets[14])
{
	copy(&mOffsets[0], &mOffsets[14], &offsets[0]);
}

vector<float> SSAO::CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	int blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= maxBlurRadius);

	vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}
	return weights;
}

ID3D12Resource* SSAO::NormalMap()
{
	return mNormalMap.Get();
}

ID3D12Resource* SSAO::AmbientMap()
{
	return mAmbientMap0.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SSAO::NormalMapRtv() const
{
	return mhNormalMapCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SSAO::NormalMapSrv() const
{
	return mhNormalMapGpuSrv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SSAO::AmbientMapSrv() const
{
	return mhAmbientMap0GpuSrv;
}

void SSAO::BuildDescriptors(ID3D12Resource* depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT cbvSrvUavDescriptorSize, UINT rtvDescriptorSize)
{
	mhAmbientMap0CpuSrv = hCpuSrv;
	mhAmbientMap1CpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhNormalMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhDepthMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhRandomVectorMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);

	mhAmbientMap0GpuSrv = hGpuSrv;
	mhAmbientMap1GpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhNormalMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhDepthMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhRandomVectorMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);

	mhNormalMapCpuRtv = hCpuRtv;
	mhAmbientMap0CpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);
	mhAmbientMap1CpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);

	RebuildDescriptors(depthStencilBuffer);
}

void SSAO::RebuildDescriptors(ID3D12Resource* depthStencilBuffer)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = normalMapFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	mDevice->CreateShaderResourceView(mNormalMap.Get(), &srvDesc, mhNormalMapCpuSrv);

	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	mDevice->CreateShaderResourceView(depthStencilBuffer, &srvDesc, mhDepthMapCpuSrv);

	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	mDevice->CreateShaderResourceView(mRandomVectorMap.Get(), &srvDesc, mhRandomVectorMapCpuSrv);

	srvDesc.Format = ambientMapFormat;
	mDevice->CreateShaderResourceView(mAmbientMap0.Get(), &srvDesc, mhAmbientMap0CpuSrv);
	mDevice->CreateShaderResourceView(mAmbientMap1.Get(), &srvDesc, mhAmbientMap1CpuSrv);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = normalMapFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	mDevice->CreateRenderTargetView(mNormalMap.Get(), &rtvDesc, mhNormalMapCpuRtv);

	rtvDesc.Format = ambientMapFormat;
	mDevice->CreateRenderTargetView(mAmbientMap0.Get(), &rtvDesc, mhAmbientMap0CpuRtv);
	mDevice->CreateRenderTargetView(mAmbientMap1.Get(), &rtvDesc, mhAmbientMap1CpuRtv);

}

void SSAO::SetPSOs(ID3D12PipelineState* ssaoPso, ID3D12PipelineState* ssaoBlurPso)
{
	mSsaoPso = ssaoPso;
	mBlurPso = ssaoBlurPso;
}

void SSAO::OnResize(UINT newWidth, UINT newHeight)
{
	if (mWidth != newWidth || mHeight != newHeight) {
		mWidth = newWidth;
		mHeight = newHeight;

		mViewport.TopLeftX = 0.0f;
		mViewport.TopLeftY = 0.0f;
		mViewport.Width = mWidth / 2.0f;
		mViewport.Height = mHeight / 2.0f;
		mViewport.MinDepth = 0.0f;
		mViewport.MaxDepth = 1.0f;

		mScissorRect = { 0,0,(int)mWidth / 2,(int)mHeight / 2 };

		BuildResource();
	}
}

void SSAO::ComputeSsao(ID3D12GraphicsCommandList* list, FrameResource* currFrame, int blurCount)
{
	list->RSSetViewports(1, &mViewport);
	list->RSSetScissorRects(1, &mScissorRect);

	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f,1.0f,1.0f,1.0f };
	list->ClearRenderTargetView(mhAmbientMap0CpuRtv, clearValue, 0, nullptr);

	list->OMSetRenderTargets(1, &mhAmbientMap0CpuRtv, true, nullptr);

	auto ssaoCBAddress = currFrame->ssaoCB->Resource()->GetGPUVirtualAddress();
	list->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);
	list->SetGraphicsRoot32BitConstant(1, 0, 0);

	list->SetGraphicsRootDescriptorTable(2, mhNormalMapGpuSrv);

	list->SetGraphicsRootDescriptorTable(3, mhRandomVectorMapGpuSrv);

	list->IASetVertexBuffers(0, 0, nullptr);
	list->IASetIndexBuffer(nullptr);
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(6, 1, 0, 0);

	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	BlurAmbientMap(list, currFrame, blurCount);

}

void SSAO::BlurAmbientMap(ID3D12GraphicsCommandList* list, FrameResource* currFrame, int blurCount)
{
	list->SetPipelineState(mBlurPso);

	auto ssaoCBAddress = currFrame->ssaoCB->Resource()->GetGPUVirtualAddress();
	list->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

	for (int i = 0; i < blurCount; i++)
	{
		BlurAmbientMap(list, true);
		BlurAmbientMap(list, false);
	}
}

void SSAO::BlurAmbientMap(ID3D12GraphicsCommandList* list, bool horizontalBlur)
{
	ID3D12Resource* output = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

	if (horizontalBlur == true) {
		output = mAmbientMap1.Get();
		inputSrv = mhAmbientMap0GpuSrv;
		outputRtv = mhAmbientMap1CpuRtv;
		list->SetGraphicsRoot32BitConstant(1, 1, 0);
	}
	else {
		output = mAmbientMap0.Get();
		inputSrv = mhAmbientMap1GpuSrv;
		outputRtv = mhAmbientMap0CpuRtv;
		list->SetGraphicsRoot32BitConstant(1, 0, 0);
	}

	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f,1.0f,1.0f,1.0f };
	list->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);

	list->OMSetRenderTargets(1, &outputRtv, true, nullptr);

	list->SetGraphicsRootDescriptorTable(2, mhNormalMapGpuSrv);

	list->SetGraphicsRootDescriptorTable(3, inputSrv);

	list->IASetVertexBuffers(0, 0, nullptr);
	list->IASetIndexBuffer(nullptr);
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(6, 1, 0, 0);

	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SSAO::BuildResource()
{
	mNormalMap = nullptr;
	mAmbientMap0 = nullptr;
	mAmbientMap1 = nullptr;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = normalMapFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	float normalClearColor[] = { 0.0f,0.0f,1.0f,0.0f };
	CD3DX12_CLEAR_VALUE optClear(normalMapFormat, normalClearColor);
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE, 
		&texDesc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		&optClear, 
		IID_PPV_ARGS(&mNormalMap)));

	texDesc.Width = mWidth / 2;
	texDesc.Height = mHeight / 2;
	texDesc.Format = ambientMapFormat;

	float ambientClearColor[] = { 1.0f,1.0f,1.0f,1.0f };
	optClear = CD3DX12_CLEAR_VALUE(ambientMapFormat, ambientClearColor);

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mAmbientMap0)));

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mAmbientMap1)));
}

void SSAO::BuildRandomVectorTexture(ID3D12GraphicsCommandList* list)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	float normalClearColor[] = { 0.0f,0.0f,1.0f,0.0f };
	CD3DX12_CLEAR_VALUE optClear(normalMapFormat, normalClearColor);
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mRandomVectorMap)));

	const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mRandomVectorMap.Get(), 0, num2DSubresources);

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mRandomVectorMapUploadBuffer.GetAddressOf())));

	XMCOLOR initData[256 * 256];
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());
			
			initData[i * 256 + j] = XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
	subResourceData.SlicePitch = subResourceData.RowPitch * 256;

	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRandomVectorMap.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(list, mRandomVectorMap.Get(), mRandomVectorMapUploadBuffer.Get(), 0, 0, num2DSubresources, &subResourceData);
	list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRandomVectorMap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void SSAO::BuildOffsetVectors()
{
	mOffsets[0] = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	mOffsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[2] = XMFLOAT4(-1.0f, 1.0f, 1.0f, 0.0f);
	mOffsets[3] = XMFLOAT4(1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[4] = XMFLOAT4(1.0f, 1.0f, -1.0f, 0.0f);
	mOffsets[5] = XMFLOAT4(-1.0f, -1.0f, 1.0f, 0.0f);

	mOffsets[6] = XMFLOAT4(-1.0f, 1.0f, -1.0f, 0.0f);
	mOffsets[7] = XMFLOAT4(1.0f, -1.0f, 1.0f, 0.0f);

	mOffsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	mOffsets[9] = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);

	mOffsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	mOffsets[11] = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	mOffsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	mOffsets[13] = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

	for (int i = 0; i < 14; i++)
	{
		float s = MathHelper::RandF(0.25f, 1.0f);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));

		XMStoreFloat4(&mOffsets[i], v);
	}
}



