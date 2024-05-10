#pragma once

#include "DxException.h"
#include "FrameResource.h"

class SSAO {
public:
	SSAO(ID3D12Device* device, ID3D12GraphicsCommandList* list, UINT width, UINT height);
	SSAO(const SSAO& rhs) = delete;
	SSAO& operator=(const SSAO& rhs) = delete;
	~SSAO() = default;

	static const DXGI_FORMAT ambientMapFormat = DXGI_FORMAT_R16_UNORM;
	static const DXGI_FORMAT normalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	static const int maxBlurRadius = 5;

	UINT Width()const;
	UINT Height()const;

	void GetOffsetVectors(XMFLOAT4 offsets[14]);
	vector<float> CalcGaussWeights(float sigma);

	ID3D12Resource* NormalMap();
	ID3D12Resource* AmbientMap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv()const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv()const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE AmbientMapSrv()const;

	void BuildDescriptors(ID3D12Resource* depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT cbvSrvUavDescriptorSize, UINT rtvDescriptorSize);
	
	void RebuildDescriptors(ID3D12Resource* depthStencilBuffer);

	void SetPSOs(ID3D12PipelineState* ssaoPso, ID3D12PipelineState* ssaoBlurPso);
	
	void OnResize(UINT newWidth, UINT newHeight);

	void ComputeSsao(ID3D12GraphicsCommandList* list, FrameResource* currFrame, int blurCount);

private:
	void BlurAmbientMap(ID3D12GraphicsCommandList* list, FrameResource* currFrame, int blurCount);
	void BlurAmbientMap(ID3D12GraphicsCommandList* list, bool horizontalBlur);

	void BuildResource();
	void BuildRandomVectorTexture(ID3D12GraphicsCommandList* list);

	void BuildOffsetVectors();

private:
	ID3D12Device* mDevice;

	ID3D12PipelineState* mSsaoPso = nullptr;
	ID3D12PipelineState* mBlurPso = nullptr;

	ComPtr<ID3D12Resource> mRandomVectorMap;
	ComPtr<ID3D12Resource> mRandomVectorMapUploadBuffer;
	ComPtr<ID3D12Resource> mNormalMap;
	ComPtr<ID3D12Resource> mAmbientMap0;
	ComPtr<ID3D12Resource> mAmbientMap1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhDepthMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhDepthMapGpuSrv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap0GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap1GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuRtv;

	UINT mWidth = 0;
	UINT mHeight = 0;

	XMFLOAT4 mOffsets[14];

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
};
