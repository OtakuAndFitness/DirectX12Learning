#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <windowsx.h>
#include <comdef.h>
#include "d3dx12.h"
#include "DDSTextureLoader.h"

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;

const int frameResourceCount = 3;

const XMFLOAT4X4 IdentityMatrix(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f);

inline wstring AnsiToWString(const string& str) {
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return wstring(buffer);
}

class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const wstring& functionName, const wstring& fileName, int lineNumber);

	wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	wstring FunctionName;
	wstring FileName;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x){ \
	HRESULT hr__ = (x); \
	wstring wfn = AnsiToWString(__FILE__); \
	if (FAILED(hr__)){ \
		throw DxException(hr__, L#x, wfn, __LINE__); \
	} \
}
#endif // !ThrowIfFailed

class d3dUtil {
public:
	static UINT CalcConstantBufferByteSize(UINT byteSize) {
		return (byteSize + 255) & ~255;
	}

	static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
	static ComPtr<ID3DBlob> CompileShader(const wstring& filename, const D3D_SHADER_MACRO* defines, const string& entrypoint, const string& target);
};

struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	BoundingBox Bounds;
};

struct MeshGeometry {
	string Name;

	ComPtr<ID3DBlob> VertexBufferCPU = nullptr;//CPU系统内存上的顶点数据
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;//CPU系统内存上的索引数据

	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;//GPU默认堆中的索引缓冲区（最终的GPU索引缓冲区）
	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;//GPU默认堆中的顶点缓冲区（最终的GPU顶点缓冲区）

	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;//GPU上传堆中的顶点缓冲区
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;//GPU上传堆中的索引缓冲区

	unordered_map<string, SubmeshGeometry> DrawArgs;//对应不同子物体绘制三参数的无序列表

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT IndexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;

	UINT IndexCount = 0;

	D3D12_INDEX_BUFFER_VIEW GetIbv()const {
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	D3D12_VERTEX_BUFFER_VIEW GetVbv()const {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
		vbv.StrideInBytes = VertexByteStride;//每个顶点元素所占用的字节数
		vbv.SizeInBytes = VertexBufferByteSize;//顶点缓冲区大小（所有顶点数据大小）

		return vbv;
	}

	//等上传堆资源传至默认堆后,释放上传堆里的内存
	void DisposeUploaders() {
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

struct Material {
	string name;
	int matCBIndex = -1;//材质常量缓冲区中的索引
	int numFramesDirty = frameResourceCount;//已更新标志，表示材质已有变动，我们需要更新常量缓冲区了
	int diffuseSrvHeapIndex = -1;

	XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };//材质反照率
	XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };//RF(0)值，即材质的反射属性
	float roughness = 0.25f;//材质的粗糙度
	
	XMFLOAT4X4 matTransform = IdentityMatrix;
};

#define MaxLights 16

struct Light {
	XMFLOAT3 strength = { 0.5f, 0.5f, 0.5f };
	float falloffStart = 1.0f;
	XMFLOAT3 direction = { 0.0f,-1.0f,0.0f };
	float falloffEnd = 0.0f;
	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	float spotPower = 64.0f;
};

struct Texture
{
	string name;//纹理名
	wstring fileName;//纹理所在路径的目录名
	ComPtr<ID3D12Resource> resource = nullptr;//返回的纹理资源
	ComPtr<ID3D12Resource> uploadHeap = nullptr;//返回的上传堆中的纹理资源
};