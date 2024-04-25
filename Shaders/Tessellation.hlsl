
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

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo; //材质反照率
    float3 gFresnelR0; //RF(0)值，即材质的反射属性
    float gRoughness; //材质的粗糙度
    float4x4 gMatTransform; //UV动画变换矩阵
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

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;

};

struct PatchTess
{
    float edgeTess[4] : SV_TessFactor;
    float insideTess[2] : SV_InsideTessFactor;
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

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL) * 0.25f;
    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
    float d = distance(centerW, gEyePosW);
    
    float d0 = 20.0f;
    float d1 = 100.0f;
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));
    
    pt.edgeTess[0] = tess;
    pt.edgeTess[1] = tess;
    pt.edgeTess[2] = tess;
    pt.edgeTess[3] = tess;
    pt.insideTess[0] = tess;
    pt.insideTess[1] = tess;
    
    return pt;

}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> patch, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
    HullOut hout;
    hout.PosL = patch[i].PosL;
    return hout;
}

[domain("quad")]
DomainOut DS(PatchTess pt, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> patch)
{
    DomainOut dout;
    
    float3 v1 = lerp(patch[0].PosL, patch[1].PosL, uv.x);
    float3 v2 = lerp(patch[2].PosL, patch[3].PosL, uv.x);
    float3 v = lerp(v1, v2, uv.y);
    
    v.y = 0.3f * (v.z * sin(v.x) + v.x * cos(v.z));
    
    float4 posW = mul(float4(v, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
    
    return dout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
    
}