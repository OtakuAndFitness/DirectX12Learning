#include "BlurFilter.h"

BlurFilter::BlurFilter(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mFormat = format;

	BuildResources();
}

void BlurFilter::BuildResources()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;//必须用此标志才能和UAV绑定

	//创建一个资源和一个堆，并将资源提交至堆中（将纹理资源blurMap0提交至GPU显存中）
	ThrowIfFailed(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE, 
		&texDesc, 
		D3D12_RESOURCE_STATE_COMMON, //资源的状态为初始状态
		nullptr, 
		IID_PPV_ARGS(&mBlurMap0)));//返回blurMap0纹理资源

	ThrowIfFailed(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON, //资源的状态为初始状态
		nullptr,
		IID_PPV_ARGS(&mBlurMap1)));//返回blurMap0纹理资源
}

void BlurFilter::BuildDescriptors()
{
	//分别创建两个纹理的SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	md3dDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur0CpuSrv);
	md3dDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur1CpuSrv);


	//分别创建两个纹理的UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = mFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	md3dDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur0CpuUav);
	md3dDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur1CpuUav);
}

ID3D12Resource* BlurFilter::Output()
{
	return mBlurMap0.Get();
}

void BlurFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, UINT descriptorSize)
{
	//计算所有描述符的地址（CPU上）
	mBlur0CpuSrv = cpuDescriptor;
	mBlur0CpuUav = cpuDescriptor.Offset(1, descriptorSize);
	mBlur1CpuSrv = cpuDescriptor.Offset(1, descriptorSize);
	mBlur1CpuUav = cpuDescriptor.Offset(1, descriptorSize);

	//计算所有描述符的地址（GPU上）
	mBlur0GpuSrv = gpuDescriptor;
	mBlur0GpuUav = gpuDescriptor.Offset(1, descriptorSize);
	mBlur1GpuSrv = gpuDescriptor.Offset(1, descriptorSize);
	mBlur1GpuUav = gpuDescriptor.Offset(1, descriptorSize);

	BuildDescriptors();
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* horizontalBlurPso, ID3D12PipelineState* verticalBlurPso, ID3D12Resource* input, int blurCount)
{
	auto weights = CalcGaussWeights(2.5f);//计算高斯核
	int blurRadius = (int)weights.size() / 2;//反向计算模糊长度
	//设置根签名和根常量
	cmdList->SetComputeRootSignature(rootSig);//绑定传至计算着色器的根签名
	
	//设置“模糊半径”的根常量
	cmdList->SetComputeRoot32BitConstants(
		0, //根参数索引
		1, //根常量数组元素个数
		&blurRadius,//根常量数组 
		0);//首元素位于根常量中位置

	//设置“高斯核权重”的根常量
	cmdList->SetComputeRoot32BitConstants(
		0,
		(UINT)weights.size(), 
		weights.data(), 
		1);
	
	//将后台缓冲区资源转换成“复制源”状态
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	
	//将blurMap0离屏纹理资源转换成“复制目标”状态
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	
	//将后台缓冲纹理拷贝至blurMap0离屏纹理中
	cmdList->CopyResource(mBlurMap0.Get(), input);
	
	// 将blurMap0离屏纹理资源转换成“可读”状态
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	// 将blurMap1离屏纹理资源转换成“可写入”状态
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	for (int i = 0; i < blurCount; i++)
	{
		//执行横向模糊
		cmdList->SetPipelineState(horizontalBlurPso);//执行横向计算着色器

		cmdList->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);//blurMap0作为输入
		cmdList->SetGraphicsRootDescriptorTable(2, mBlur1GpuUav);//blurMap1作为输出

		UINT numGroupsX = (UINT)ceilf(mWidth / 256.0f);//X方向的线程组数量
		cmdList->Dispatch(numGroupsX, mHeight, 1);//分派线程组(之后便执行计算着色器)
		// 将blurMap0离屏纹理资源转换成“可写入”状态
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		// 将blurMap1离屏纹理资源转换成“可读”状态
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		
		//执行纵向模糊
		cmdList->SetPipelineState(verticalBlurPso);//执行纵向计算着色器

		cmdList->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);//blurMap1作为输入
		cmdList->SetGraphicsRootDescriptorTable(2, mBlur0GpuUav);//blurMap0作为输出

		UINT numGroupsY = (UINT)ceilf(mHeight / 256.0f);//Y方向的线程组数量
		cmdList->Dispatch(numGroupsY, mWidth, 1);//分派线程组(之后便执行计算着色器)
		// 将blurMap0离屏纹理资源转换成“可写入”状态
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		// 将blurMap1离屏纹理资源转换成“可读”状态
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	}

}

void BlurFilter::OnResize(UINT newWidth, UINT newHeight)
{
	mWidth = newWidth;
	mHeight = newHeight;
	//以新的大小来重建离屏纹理资源
	BuildResources();
	//既然创建了新的资源，也应当为其创建新的描述符
	BuildDescriptors();
}

vector<float> BlurFilter::CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;//高斯核计算公式的分母

	int blurRadius = (int)ceil(2.0f * sigma);//通过sigma计算模糊半径

	assert(blurRadius <= MaxBlurRadius);

	vector<float> weights;//定义高斯核权重矩阵
	weights.resize(2 * blurRadius + 1);//高斯核总长度

	float weightSum = 0.0f;//权重和

	//计算每个权重（未归一化）
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		float x = (float)i;
		weights[i + blurRadius] = expf(-x * x / twoSigma2);//计算每个权重
		weightSum += weights[i + blurRadius];//计算权重和
	}

	//归一化权重
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}
	
	return weights;
}
