#include "DxException.h"

DxException::DxException(HRESULT hr, const wstring& functionName, const wstring& fileName, int lineNumber) : ErrorCode(hr), FunctionName(functionName), FileName(fileName), LineNumber(lineNumber) {

}

wstring DxException::ToString()const {
	_com_error err(ErrorCode);
	wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + FileName + L"; line " + to_wstring(LineNumber) + L"; error: " + msg;
}