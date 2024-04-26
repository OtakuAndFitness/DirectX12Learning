
#include "LightingUtils.hlsl"

Texture2D gDiffuseMap : register(t0); //所有漫反射贴图

//6个不同类型的采样器
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWrap : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform; //UV顶点变换矩阵
};

cbuffer cbPass : register(b1)
{
    float4x4 gViewProj;
    float3 gEyePosW;
    float gTotalTime;
    float4 gAmbientLight;
    Light gLights[MaxLights];
    
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 pad2;
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo; //材质反照率
    float3 gFresnelR0; //RF(0)值，即材质的反射属性
    float gRoughness; //材质的粗糙度
    float4x4 gMatTransform; //UV动画变换矩阵
};

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;

};
//曲面细分因子
struct PatchTess
{
    float edgeTess[4] : SV_TessFactor; //4条边的细分因子
    float insideTess[2] : SV_InsideTessFactor; //内部的细分因子（u和v两个方向）
};

struct HullOut
{
    float3 PosL : POSITION;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.PosL = vin.PosL;
    
    return vout;
}

float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;
    
    return float4(invT * invT * invT,
                        3.0f * t * invT * invT,
                        3.0f * t * t * invT,
                        t * t * t);
}

float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezpatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum = basisV.x * (basisU.x * bezpatch[0].PosL + basisU.y * bezpatch[1].PosL + basisU.z * bezpatch[2].PosL + basisU.w * bezpatch[3].PosL);
    sum += basisV.y * (basisU.x * bezpatch[4].PosL + basisU.y * bezpatch[5].PosL + basisU.z * bezpatch[6].PosL + basisU.w * bezpatch[7].PosL);
    sum += basisV.z * (basisU.x * bezpatch[8].PosL + basisU.y * bezpatch[9].PosL + basisU.z * bezpatch[10].PosL + basisU.w * bezpatch[11].PosL);
    sum += basisV.w * (basisU.x * bezpatch[12].PosL + basisU.y * bezpatch[13].PosL + basisU.z * bezpatch[14].PosL + basisU.w * bezpatch[15].PosL);

    return sum;
    

}

//PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, //输入的面片控制点
//                                    uint patchID : SV_PrimitiveID)//面片图元ID
//{
//    PatchTess pt;
//    //计算出面片的中点
//    float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL) * 0.25f;
//    //将中心点从模型空间转到世界空间下
//    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
//    //计算摄像机和面片的距离
//    float d = distance(centerW, gEyePosW);
//    //LOD的变化区间（最大和最小区间）
//    float d0 = 20.0f;
//    float d1 = 100.0f;
//     //随着距离变化，计算细分因子,注意先减后除的括号！！！！！
//    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));
//    //赋值所有边和内部的细分因子
//    pt.edgeTess[0] = tess;
//    pt.edgeTess[1] = tess;
//    pt.edgeTess[2] = tess;
//    pt.edgeTess[3] = tess;
//    pt.insideTess[0] = tess;
//    pt.insideTess[1] = tess;
    
//    return pt;

//}

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, //输入的面片控制点
                                    uint patchID : SV_PrimitiveID)//面片图元ID
{
    PatchTess pt;
    
    pt.edgeTess[0] = 25;
    pt.edgeTess[1] = 25;
    pt.edgeTess[2] = 25;
    pt.edgeTess[3] = 25;
    pt.insideTess[0] = 25;
    pt.insideTess[1] = 25;
    
    return pt;

}

//[domain("quad")] //传入的patch为四边形面片
//[partitioning("integer")] //细分模式为整型
//[outputtopology("triangle_cw")] //三角形绕序为顺时针
//[outputcontrolpoints(4)] //HS执行次数
//[patchconstantfunc("ConstantHS")] //所执行的“常量外壳着色器名”
//[maxtessfactor(64.0f)] //细分因子最大值
//HullOut HS(InputPatch<VertexOut, 4> patch, //控制点
//                    uint i : SV_OutputControlPointID, //正被执行的控制点索引
//                    uint patchID : SV_PrimitiveID)//图元索引
//{
//    HullOut hout;
//    hout.PosL = patch[i].PosL; //仅传值，控制点数量不变
//    return hout;
//}

[domain("quad")] //传入的patch为四边形面片
[partitioning("integer")] //细分模式为整型
[outputtopology("triangle_cw")] //三角形绕序为顺时针
[outputcontrolpoints(16)] //HS执行次数
[patchconstantfunc("ConstantHS")] //所执行的“常量外壳着色器名”
[maxtessfactor(64.0f)] //细分因子最大值
HullOut HS(InputPatch<VertexOut, 16> patch, //控制点
                    uint i : SV_OutputControlPointID, //正被执行的控制点索引
                    uint patchID : SV_PrimitiveID)//图元索引
{
    HullOut hout;
    hout.PosL = patch[i].PosL; //仅传值，控制点数量不变
    return hout;
}

//[domain("quad")]
//DomainOut DS(PatchTess pt, //细分因子
//                        float2 uv : SV_DomainLocation, //细分后顶点UV（位置UV，非纹理UV）
//                        const OutputPatch<HullOut, 4> patch)//patch的4个控制点
//{
//    DomainOut dout;
    
//    float3 v1 = lerp(patch[0].PosL, patch[1].PosL, uv.x);
//    float3 v2 = lerp(patch[2].PosL, patch[3].PosL, uv.x);
//    float3 v = lerp(v1, v2, uv.y);
    
//    v.y = 0.3f * (v.z * sin(v.x) + v.x * cos(v.z));
    
//    float4 posW = mul(float4(v, 1.0f), gWorld);
//    dout.PosH = mul(posW, gViewProj);
    
//    return dout;
//}

[domain("quad")]
DomainOut DS(PatchTess pt, //细分因子
                        float2 uv : SV_DomainLocation, //细分后顶点UV（位置UV，非纹理UV）
                        const OutputPatch<HullOut, 16> patch)//patch的4个控制点
{
    DomainOut dout;
    
    float4 basisU = BernsteinBasis(uv.x);
    float4 basisV = BernsteinBasis(uv.y);
    
    float3 p = CubicBezierSum(patch, basisU, basisV);
    
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
    
    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    return gDiffuseAlbedo;
    
}