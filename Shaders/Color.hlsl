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
    uint gMatPad0;
    uint gMatPad1;
    uint gMatPad2;

};

Texture2D gDiffuseMap[4] : register(t0); //所有漫反射贴图
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

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
    uint gMaterialDataIndex;
    uint gObjPad0;
    uint gObjPad1;
    uint gObjPad2;

};

//cbuffer cbMaterial : register(b2)
//{
//    float4 gDiffuseAlbedo; //材质反照率
//    float3 gFresnelR0; //RF(0)值，即材质的反射属性
//    float gRoughness; //材质的粗糙度
//    float4x4 gMatTransform; //UV动画变换矩阵
//};

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
    //float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    //float4 Color : COLOR;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 uv : TEXCOORD;

};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    MaterialData matData = gMaterialData[gMaterialDataIndex];
	
    float3 posW = mul(float4(vin.PosL, 1.0F), gWorld).xyz;
    vout.PosW = posW;
    
    vout.NormalW = mul(vin.Normal, (float3x3) gWorld);
    
    vout.PosH = mul(float4(posW, 1.0f), gViewProj);
	
    //vout.Color = vin.Color;
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    vout.uv = mul(texCoord, matData.gMatTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[gMaterialDataIndex];
    float4 diffuseAlbedo = matData.gDiffuseAlbedo;
    float3 fresnelR0 = matData.gFresnelR0;
    float roughtness = matData.gRoughness;
    uint diffuseTexIndex = matData.gDiffuseMapIndex;
    
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gSamAnisotropicWarp, pin.uv);
    
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    float3 worldNormal = normalize(pin.NormalW);
    //float3 worldView = normalize(gEyePosW - pin.PosW);
    float3 worldPosToEye = gEyePosW - pin.PosW;
    float distPosToEye = length(worldPosToEye);
    float3 worldView = worldPosToEye / distPosToEye;
    
    Material mat = { diffuseAlbedo, fresnelR0, roughtness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, worldNormal, worldView, shadowFactor);
    float4 ambient = gAmbientLight * diffuseAlbedo;
    float4 finalCol = ambient + directLight;
    
#ifdef FOG
    float s = saturate((distPosToEye - gFogStart) / gFogRange);
    finalCol = lerp(finalCol, gFogColor, s);
#endif
    
    finalCol.a = diffuseAlbedo.a;
    return finalCol;
}