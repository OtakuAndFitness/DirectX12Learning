#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "MathHelper.h"

using namespace DirectX;

class Camera {
public:
	Camera();
	~Camera();
	
	//Get及Set世界空间下摄像机的位置
	XMVECTOR GetPositon()const;
	XMFLOAT3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);
	
	//获取摄像机的基向量（观察空间的三个轴在世界空间下的表示）
	XMVECTOR GetRight()const;
	XMFLOAT3 GetRight3f()const;
	XMVECTOR GetUp()const;
	XMFLOAT3 GetUp3f()const;
	XMVECTOR GetLook()const;
	XMFLOAT3 GetLook3f()const;
	
	//获取视锥体属性
	float GetNearZ()const;//近裁剪面距离
	float GetFarZ()const;//远裁剪面距离
	float GetAspect()const;//视口纵横比
	float GetFovY()const;//垂直视场角
	float GetFovX()const;//水平视场角
	
	//获取用观察空间坐标表示的近、远平面大小
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	
	//赋值视锥体变量，设置投影矩阵(实现使用了内置函数XMMatrixPerspectiveFovLH)
	void SetLens(float fovY, float aspect, float zn, float zf);
	
	//准备观察矩阵数据
	void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR up);
	void LookAt(XMFLOAT3& pos, XMFLOAT3& target, XMFLOAT3& up);
	
	//获取观察矩阵和投影矩阵
	XMMATRIX GetView()const;
	XMMATRIX GetProj()const;
	XMFLOAT4X4 GetView4x4f()const;
	XMFLOAT4X4 GetProj4x4f()const;
	
	//将摄像机按距离d左右平移（Strafe）或前后移动（Walk）
	void Strafe(float distance);//左右平移摄像机
	void Walk(float distance);//前后推拉摄像机

	//旋转摄像机
	void Pitch(float angle);
	void Yaw(float angle);

	//修改摄像机的位置和朝向后，调用此函数来重新构建观察矩阵
	void UpdateViewMatrix();

private:
	//摄像机坐标系（三个轴的基向量）在世界空间下的坐标
	XMFLOAT3 mPosition = { 0.0f, 0.0f,0.0f };
	XMFLOAT3 mRight = { 1.0f,0.0f,0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f,0.0f };
	XMFLOAT3 mLook = { 0.0f,0.0f,1.0f };
	
	//视锥体属性
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;
	
	// 观察矩阵和投影矩阵
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();


};
