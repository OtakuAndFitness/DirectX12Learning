#include "Common.hlsl"

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
    
    const float shininess = 1.0f - roughtness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, worldNormal, worldView, shadowFactor);
    float4 ambient = gAmbientLight * diffuseAlbedo;
    float4 finalCol = ambient + directLight;
    
    float3 r = reflect(-worldPosToEye, worldNormal);
    float4 reflectionCol = gCubeMap.Sample(gSamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, worldNormal, r);
    finalCol.rgb += shininess * fresnelFactor * reflectionCol.rgb;
    
#ifdef FOG
    float s = saturate((distPosToEye - gFogStart) / gFogRange);
    finalCol = lerp(finalCol, gFogColor, s);
#endif
    
    finalCol.a = diffuseAlbedo.a;
    return finalCol;
}