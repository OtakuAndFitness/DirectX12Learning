#pragma once

#include "DxException.h"

class BlurFilter {
public:
	BlurFilter(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator = (const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

	ID3D12Resource* Output();
	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, UINT descriptorSize);
	void Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* horizontalBlurPso, ID3D12PipelineState* verticalBlurPso, ID3D12Resource* input, int BlurCount);
	void OnResize(UINT newWidth, UINT newHeight);
private:
	void BuildResources();
	void BuildDescriptors();
	vector<float> CalcGaussWeights(float sigma);
private:
	ComPtr<ID3D12Resource> mBlurMap0 = nullptr;
	ComPtr<ID3D12Resource> mBlurMap1 = nullptr;

	ID3D12Device* md3dDevice = nullptr;
	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav;

	const int MaxBlurRadius = 5;
};