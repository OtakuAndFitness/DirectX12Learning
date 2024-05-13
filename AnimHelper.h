#pragma once

#include "DxException.h"
#include "MathHelper.h"

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

struct AnimationClip {
	float GetClipStartTime()const;
	float GetClipEndTime()const;

	void Interpolate(float t, vector<XMFLOAT4X4>& boneTransform)const;

	vector<BoneAnimation> BoneAnimations;
};

class SkinnedData {
public:
	UINT BoneCount()const;

	float GetClipStartTime(const string& clipName)const;
	float GetClipEndTime(const string& clipName)const;

	void Set(vector<int>& boneHierarchy, vector<XMFLOAT4X4>& boneOffsets, unordered_map<string, AnimationClip>& animations);

	void GetFinalTransforms(const string& clipName, float timePos, vector<XMFLOAT4X4>& finalTransforms)const;

private:
	vector<int> mBoneHierarchy;
	vector<XMFLOAT4X4> mBoneOffsets;
	unordered_map<string, AnimationClip> mAnimations;
};
