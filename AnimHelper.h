#pragma once

#include "DxException.h"

using namespace DirectX;

struct Keyframe{
	Keyframe();
	~Keyframe();

	float TimePos;// 关键帧的时间点
	XMFLOAT3 Translation;// 关键帧的位置
	XMFLOAT3 Scale;// 关键帧的缩放
	XMFLOAT4 RotationQuat;// 关键帧的旋转
};

struct BoneAnimation {
	float GetStartTime()const;// 第一个关键帧的起始时间点
	float GetEndTime()const;// 最后一个关键帧的结束时间点

	void Interpolate(float t, XMFLOAT4X4& M)const;// 关键帧插值函数

	vector<Keyframe> Keyframes;
};
