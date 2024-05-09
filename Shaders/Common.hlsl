#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


#include "LightingUtils.hlsl"

struct MaterialData
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
    uint gDiffuseMapIndex;
    uint gNormalMapIndex;
    uint gMatPad1;
    uint gMatPad2;

};

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);
//Texture2D gDiffuseMap[4] : register(t1); //所有漫反射贴图
Texture2D gTextureMaps[10] : register(t2);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

//6个不同类型的采样器
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWrap : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform; //UV顶点变换矩阵
    uint gMaterialDataIndex;
    uint gObjPad0;
    uint gObjPad1;
    uint gObjPad2;

};

cbuffer cbPass : register(b1)
{
    float4x4 gViewProj;
    float3 gEyePosW;
    float gTotalTime;
    float2 gRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float4x4 gShadowTransform;
    float4 gAmbientLight;
    Light gLights[MaxLights];
    
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 pad2;
};

float3 NormalMapSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    
    float3x3 TBN = float3x3(T, B, N);
    
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return bumpedNormalW;
}

float CalcShadowFactor(float4 shadowPosH)
{
    // 将顶点变换到NDC空间（如果是正交投影，则W=1）
    shadowPosH.xyz /= shadowPosH.w;
    // NDC空间中的深度值
    float depth = shadowPosH.z;
    // 读取ShadowMap的宽高及mip级数
    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    
    float dx = 1.0f / (float) width;
    
    float precentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
         float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
         float2(-dx, dx), float2(0.0f, dx), float2(dx, dx),
    };
    
    [unroll]
    for (int i = 0;i <9;i++)
    {
        precentLit += gShadowMap.SampleCmpLevelZero(gsamShadow, shadowPosH.xy + offsets[i], depth).r;
    }
    
    return precentLit / 9.0f;

}