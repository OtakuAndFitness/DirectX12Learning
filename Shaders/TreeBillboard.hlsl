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
    float3 vertexW : POSITION;
    float2 size : SIZE;
};

struct VertexOut
{
    float3 centerPosW : POSITION;
    float2 sizeW : SIZE;

};

struct GeoOut
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 texC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.centerPosW = vin.vertexW;
    vout.sizeW = vin.size;
    
    return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1], inout TriangleStream<GeoOut> triStream)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - gin[0].centerPosW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up, look);
    
    float halfWidth = 0.5 * gin[0].sizeW.x;
    float halfHeight = 0.5 * gin[0].sizeW.y;
    
    float4 v[4];
    v[0] = float4(gin[0].centerPosW + right * halfWidth - up * halfHeight, 1.0f);
    v[1] = float4(gin[0].centerPosW + right * halfWidth + up * halfHeight, 1.0f);
    v[2] = float4(gin[0].centerPosW - right * halfWidth - up * halfHeight, 1.0f);
    v[3] = float4(gin[0].centerPosW - right * halfWidth + up * halfHeight, 1.0f);

    float2 texCoord[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };
    
    GeoOut geoOut;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        geoOut.posH = mul(v[i], gViewProj);
        geoOut.posW = v[i];
        geoOut.normalW = up;
        geoOut.texC = texCoord[i];
        
        triStream.Append(geoOut);
    }

}

float4 PS(GeoOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gSamAnisotropicWarp, pin.texC) * gDiffuseAlbedo;
    
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    float3 worldNormal = normalize(pin.normalW);
    //float3 worldView = normalize(gEyePosW - pin.PosW);
    float3 worldPosToEye = gEyePosW - pin.posW;
    float distPosToEye = length(worldPosToEye);
    float3 worldView = worldPosToEye / distPosToEye;
    
    Material mat = { diffuseAlbedo, gFresnelR0, gRoughness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.posW, worldNormal, worldView, shadowFactor);
    float4 ambient = gAmbientLight * diffuseAlbedo;
    float4 finalCol = ambient + directLight;
    
#ifdef FOG
    float s = saturate((distPosToEye - gFogStart) / gFogRange);
    finalCol = lerp(finalCol, gFogColor, s);
#endif
    
    finalCol.a = diffuseAlbedo.a;
    return finalCol;
}