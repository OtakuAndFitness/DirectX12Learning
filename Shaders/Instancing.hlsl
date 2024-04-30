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

struct InstanceData
{
    float4x4 world;
    float4x4 texTransform;

    uint materialIndex;
    uint objPad0;
    uint objPad1;
    uint objPad2;
};

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

Texture2D gDiffuseMap[7] : register(t0);
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

//6个不同类型的采样器
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWrap : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);

cbuffer cbPass : register(b0)
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
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 uv : TEXCOORD;

    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
    
    InstanceData insData = gInstanceData[instanceID];
    float4x4 world = insData.world;
    float4x4 texTransform = insData.texTransform;
    uint matIndex = insData.materialIndex;
    
    vout.MatIndex = matIndex;
    
    MaterialData matData = gMaterialData[matIndex];
	
    float3 posW = mul(float4(vin.PosL, 1.0F), world).xyz;
    vout.PosW = posW;
    
    vout.NormalW = mul(vin.Normal, (float3x3)world);
    
    vout.PosH = mul(float4(posW, 1.0f), gViewProj);
	
    //vout.Color = vin.Color;
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), texTransform);
    vout.uv = mul(texCoord, matData.gMatTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[pin.MatIndex];
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