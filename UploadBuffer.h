#pragma once

#include "DxException.h"

template<typename T>
class UploadBuffer {
public :
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);//如果不是常量缓冲区，则直接计算缓存大小

        if (isConstantBuffer)
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));//如果是常量缓冲区，则以256的倍数计算缓存大小
        //创建上传堆和资源
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));
        //返回欲更新资源的指针
        ThrowIfFailed(mUploadBuffer->Map(
            0, //子资源索引，对于缓冲区来说，他的子资源就是自己
            nullptr,//对整个资源进行映射
            reinterpret_cast<void**>(&mMappedData)));//返回待映射资源数据的目标内存块	
    }

    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);//取消映射

        mMappedData = nullptr;//释放映射内存
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
    }

private:
    ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;
    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;

};