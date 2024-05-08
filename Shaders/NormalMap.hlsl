#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    //float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    //float4 Color : COLOR;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 uv : TEXCOORD;
    float3 TangentW : TANGENT;

};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    MaterialData matData = gMaterialData[gMaterialDataIndex];
	
    float3 posW = mul(float4(vin.PosL, 1.0F), gWorld).xyz;
    vout.PosW = posW;
    
    vout.NormalW = mul(vin.Normal, (float3x3) gWorld);
    
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    
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
    uint normalMapIndex = matData.gNormalMapIndex;
    
    float3 worldNormal = normalize(pin.NormalW);

    float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gSamAnisotropicWarp, pin.uv);
    float3 bumpedNormalW = NormalMapSampleToWorldSpace(normalMapSample.rgb, worldNormal, pin.TangentW);
    
    diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gSamAnisotropicWarp, pin.uv);

    float3 worldPosToEye = gEyePosW - pin.PosW;
    
    float4 ambient = gAmbientLight * diffuseAlbedo;

    
    const float shininess = 1.0f - roughtness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, bumpedNormalW, worldPosToEye, shadowFactor);
    
    float4 finalCol = ambient + directLight;
    
    float3 r = reflect(-worldPosToEye, bumpedNormalW);
    float4 reflectionCol = gCubeMap.Sample(gSamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    finalCol.rgb += shininess * fresnelFactor * reflectionCol.rgb;
    
    finalCol.a = diffuseAlbedo.a;
    return finalCol;
}