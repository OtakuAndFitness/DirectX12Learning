#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


#include "LightingUtils.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo; //材质反照率
    float3 gFresnelR0; //RF(0)值，即材质的反射属性
    float gRoughness; //材质的粗糙度
};

cbuffer cbPass : register(b2)
{
    float4x4 gViewProj;
    float3 gEyePosW;
    float gTotalTime;
    float4 gAmbientLight;
    Light gLights[MaxLights];
};

struct VertexIn
{
    float3 PosL : POSITION;
    //float4 Color : COLOR;
    float3 Normal : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    //float4 Color : COLOR;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    float3 posW = mul(float4(vin.PosL, 1.0F), gWorld).xyz;
    vout.PosW = posW;
    
    vout.NormalW = mul(vin.Normal, (float3x3) gWorld);
    
    vout.PosH = mul(float4(posW, 1.0f), gViewProj);
	
    //vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 worldNormal = normalize(pin.NormalW);
    float3 worldView = normalize(gEyePosW - pin.PosW);
    
    Material mat = { gDiffuseAlbedo, gFresnelR0, gRoughness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, worldNormal, worldView, shadowFactor);
    float4 ambient = gAmbientLight * gDiffuseAlbedo;
    float4 finalCol = ambient + directLight;
    finalCol.a = gDiffuseAlbedo.a;
    return finalCol;
}