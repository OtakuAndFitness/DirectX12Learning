#pragma once

#include "DxException.h"

class CubeRenderTarget {
public:
	CubeRenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
	CubeRenderTarget(const CubeRenderTarget& rhs) = delete;
	CubeRenderTarget& operator=(const CubeRenderTarget& rhs) = delete;
	~CubeRenderTarget() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(int index);
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* mDevice = nullptr;
	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[6];

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	ComPtr<ID3D12Resource> mCubeMap = nullptr;
};
