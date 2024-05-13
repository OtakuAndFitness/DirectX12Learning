#include "AnimHelper.h"

Keyframe::Keyframe() : TimePos(0.0f),Translation(0.0f,0.0f,0.0f),Scale(1.0f,1.0f,1.0f),RotationQuat(0.0f,0.0f,0.0f,1.0f)
{

}

Keyframe::~Keyframe()
{
}

float BoneAnimation::GetStartTime() const
{
	return Keyframes.front().TimePos;
}

float BoneAnimation::GetEndTime() const
{
	float f = Keyframes.back().TimePos;
	return f;
}
// 关键帧插值函数
void BoneAnimation::Interpolate(float t, XMFLOAT4X4& M) const
{
	// 如果t在动画开始前，则返回第一个关键帧的数据
	if (t <= Keyframes.front().TimePos) {
		// 第一个关键帧的缩放、位置、旋转（使用四元数）
		XMVECTOR S = XMLoadFloat3(&Keyframes.front().Scale);
		XMVECTOR P = XMLoadFloat3(&Keyframes.front().Translation);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.front().RotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		// 将变化过程转为矩阵
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));

	}
	// 如果t在动画结束后，则返回最后一个关键帧的数据
	else if (t >= Keyframes.back().TimePos) {
		// 最后一个关键帧的缩放、位置、旋转（使用四元数）
		XMVECTOR S = XMLoadFloat3(&Keyframes.front().Scale);
		XMVECTOR P = XMLoadFloat3(&Keyframes.front().Translation);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.front().RotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		// 将变化过程转为矩阵
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// 如果t在动画中，则返回对应两关键帧之间插值的关键帧数据
	else {
		for (UINT i = 0; i < Keyframes.size(); i++)
		{
			if (t >= Keyframes[i].TimePos && t <= Keyframes[i + 1].TimePos) {// 判断在哪两个关键帧之间
				// t在两帧间的插值百分比
				float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[i + 1].TimePos - Keyframes[i].TimePos);
				XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].Scale);
				XMVECTOR s1 = XMLoadFloat3(&Keyframes[i+1].Scale);

				XMVECTOR p0 = XMLoadFloat3(&Keyframes[i].Translation);
				XMVECTOR p1 = XMLoadFloat3(&Keyframes[i+1].Translation);

				XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].RotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&Keyframes[i+1].RotationQuat);

				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

				// 将变化过程转为矩阵
				XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}
