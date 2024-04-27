#include "Camera.h"

//using namespace DirectX;

Camera::Camera()
{
}

Camera::~Camera()
{
}

XMVECTOR Camera::GetPositon() const
{
	return XMLoadFloat3(&mPosition);
}

XMFLOAT3 Camera::GetPosition3f() const
{
	return mPosition;
}

void Camera::SetPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
	mViewDirty = true;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	mPosition = v;
	mViewDirty = true;
}

XMVECTOR Camera::GetRight() const
{
	return XMLoadFloat3(&mRight);
}

XMFLOAT3 Camera::GetRight3f() const
{
	return mRight;
}

XMVECTOR Camera::GetUp() const
{
	return XMLoadFloat3(&mUp);
}

XMFLOAT3 Camera::GetUp3f() const
{
	return mUp;
}

XMVECTOR Camera::GetLook() const
{
	return XMLoadFloat3(&mLook);
}

XMFLOAT3 Camera::GetLook3f() const
{
	return mLook;
}

float Camera::GetNearZ() const
{
	return mNearZ;
}

float Camera::GetFarZ() const
{
	return mFarZ;
}

float Camera::GetAspect() const
{
	return mAspect;
}

float Camera::GetFovY() const
{
	return mFovY;
}

float Camera::GetFovX() const
{
	float halfWidth = 0.5f * GetNearWindowWidth();
	return 2.0f * atan(halfWidth / mNearZ);
}

float Camera::GetNearWindowWidth() const
{
	return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return mNearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
	return mFarWindowHeight;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f * mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f * mFovY);

	XMMATRIX p = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, p);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR up)
{
	XMVECTOR LOOK = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR RIGHT = XMVector3Cross(up, LOOK);
	XMVECTOR UP = XMVector3Cross(LOOK, RIGHT);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, LOOK);
	XMStoreFloat3(&mRight, RIGHT);
	XMStoreFloat3(&mUp, UP);

	mViewDirty = true;
}

void Camera::LookAt(XMFLOAT3& pos, XMFLOAT3& target, XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	mViewDirty = true;

}

XMMATRIX Camera::GetView() const
{
	assert(!mViewDirty);
	return XMLoadFloat4x4(&mView);
}

XMMATRIX Camera::GetProj() const
{
	return XMLoadFloat4x4(&mProj);
}

XMFLOAT4X4 Camera::GetView4x4f() const
{
	assert(!mViewDirty);
	return mView;
}

XMFLOAT4X4 Camera::GetProj4x4f() const
{
	return mProj;
}

//左右平移摄像机（在right向量方向做平移）
void Camera::Strafe(float distance)
{
	//计算公式：position += right * d
	XMVECTOR d = XMVectorReplicate(distance);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mPosition);

	XMVECTOR pos = XMVectorMultiplyAdd(r, d, p);
	XMStoreFloat3(&mPosition, pos);

	mViewDirty = true;
}

//前后推拉摄像机（在look向量方向做平移）
void Camera::Walk(float distance)
{
	//计算公式：position += look * d
	XMVECTOR d = XMVectorReplicate(distance);
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mPosition);

	XMVECTOR pos = XMVectorMultiplyAdd(d, l, p);
	XMStoreFloat3(&mPosition, pos);

	mViewDirty = true;
}

//旋转摄像机Pitch方向(俯仰角)
void Camera::Pitch(float angle)
{
	XMVECTOR right = XMLoadFloat3(&mRight);
	XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR look = XMLoadFloat3(&mLook);

	XMMATRIX R = XMMatrixRotationAxis(right, angle);//旋转轴为right轴，得到旋转矩阵
	
	up = XMVector3TransformNormal(up, R);//矩阵变换后的up向量
	look = XMVector3TransformNormal(look, R);//矩阵变换后的look向量

	XMStoreFloat3(&mUp, up);
	XMStoreFloat3(&mLook, look);

	mViewDirty = true;
}

//旋转摄像机Yaw方向（航向角）
void Camera::Yaw(float angle)
{
	XMVECTOR right = XMLoadFloat3(&mRight);
	//XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR look = XMLoadFloat3(&mLook);

	XMMATRIX R = XMMatrixRotationY(angle);//旋转轴为Y轴，得到旋转矩阵

	right = XMVector3TransformNormal(right, R);//矩阵变换后的right向量
	look = XMVector3TransformNormal(look, R);//矩阵变换后的look向量

	XMStoreFloat3(&mRight, right);
	XMStoreFloat3(&mLook, look);

	mViewDirty = true;
}

void Camera::UpdateViewMatrix()
{
	if (mViewDirty) {
		XMVECTOR R = XMLoadFloat3(&mRight);
		XMVECTOR U = XMLoadFloat3(&mUp);
		XMVECTOR L = XMLoadFloat3(&mLook);
		XMVECTOR P = XMLoadFloat3(&mPosition);

		//以Look向量为基准，修正up和right向量
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));
		R = XMVector3Cross(U, L);

		// 填写观察矩阵中的数据元素
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&mRight, R);
		XMStoreFloat3(&mUp, U);
		XMStoreFloat3(&mLook, L);

		mView(0, 0) = mRight.x;
		mView(1, 0) = mRight.y;
		mView(2, 0) = mRight.z;
		mView(3, 0) = x;

		mView(0, 1) = mUp.x;
		mView(1, 1) = mUp.y;
		mView(2, 1) = mUp.z;
		mView(3, 1) = y;

		mView(0, 2) = mLook.x;
		mView(1, 2) = mLook.y;
		mView(2, 2) = mLook.z;
		mView(3, 2) = z;

		mView(0, 3) = 0.0f;
		mView(1, 3) = 0.0f;
		mView(2, 3) = 0.0f;
		mView(3, 3) = 1.0f;

		mViewDirty = false;
	}
}
