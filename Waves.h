#pragma once

#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

class Waves
{
public:
    Waves(int m, int n, float dx, float dt, float speed, float damping);
    Waves(const Waves& rhs) = delete;
    Waves& operator=(const Waves& rhs) = delete;
    ~Waves();

    int RowCount()const;
    int ColumnCount()const;
    int VertexCount()const;
    int TriangleCount()const;
    float Width()const;
    float Depth()const;

    // 返回计算后的网格顶点坐标
    const XMFLOAT3& Position(int i)const { return mCurrSolution[i]; }

    // 返回计算后的网格顶点法线
    const XMFLOAT3& Normal(int i)const { return mNormals[i]; }

    // 返回计算后的网格顶点切线
    const XMFLOAT3& TangentX(int i)const { return mTangentX[i]; }

    void Update(float dt);
    void Disturb(int i, int j, float magnitude);

private:
    int mNumRows = 0;
    int mNumCols = 0;

    int mVertexCount = 0;
    int mTriangleCount = 0;

    // Simulation constants we can precompute.
    float mK1 = 0.0f;
    float mK2 = 0.0f;
    float mK3 = 0.0f;

    float mTimeStep = 0.0f;
    float mSpatialStep = 0.0f;

    std::vector<XMFLOAT3> mPrevSolution;
    std::vector<XMFLOAT3> mCurrSolution;
    std::vector<XMFLOAT3> mNormals;
    std::vector<XMFLOAT3> mTangentX;
};
