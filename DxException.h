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


using namespace std;
using namespace Microsoft::WRL;

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
	static ComPtr<ID3DBlob> CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target);
};
