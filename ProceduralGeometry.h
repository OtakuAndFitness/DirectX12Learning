#pragma once

#include <DirectXMath.h>
#include <vector>

using namespace std;
using namespace DirectX;

class ProceduralGeometry {
public:
	using uint16 = uint16_t;
	using uint32 = uint32_t;

	struct Vertex {
		XMFLOAT3 Position;//物体坐标（物体空间）
		XMFLOAT3 Normal;//法线（物体空间）
		XMFLOAT3 TangentU;//切线（物体空间）
		XMFLOAT2 TexC;//纹理坐标

		Vertex() {}
		Vertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t, const XMFLOAT2& uv) : Position(p), Normal(n), TangentU(t), TexC(uv) {}
		Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v) : Position(px, py, pz), Normal(nx, ny, nz), TangentU(tx, ty, tz), TexC(u, v) {}
	};

	struct MeshData {
		vector<Vertex> Vertices; //单个几何体的顶点数组
		vector<uint32> Indices32; //单个几何体的索引数组
		
		//将32位的索引列表转换成16位
		vector<uint16>& GetIndices16() {
			if (mIndices16.empty()) {
				mIndices16.resize(Indices32.size());
				for (size_t i = 0; i < Indices32.size(); i++)
				{
					mIndices16[i] = static_cast<uint16>(Indices32[i]);
				}
			}
			return mIndices16;
		}

	private:
		vector<uint16_t> mIndices16;
	};

	MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

	MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	MeshData CreateQuad(float x, float y, float w, float h, float depth);

private:
	void Subdivide(MeshData& meshData);
	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);

};
