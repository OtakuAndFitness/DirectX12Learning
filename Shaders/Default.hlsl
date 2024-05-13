#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    //float4 Color : COLOR;
    float3 NormalL : NORMAL;
    float2 TexCoord : TEXCOORD;
    float3 TangentL : TANGENT;
#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
#endif
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH : POSITION1;
    float3 PosW : POSITION2;
    float3 NormalW : NORMAL;
    float2 uv : TEXCOORD;
    float3 TangentW : TANGENT;

};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    MaterialData matData = gMaterialData[gMaterialDataIndex];
    
#ifdef SKINNED
    float weights[4] = {0.0f,0.0f,0.0f,0.0f};
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = vin.BoneWeights.w;
    
    float3 posL = float3(0.0f,0.0f,0.0f);
    float3 normalL = float3(0.0f,0.0f,0.0f);
    float3 tangentL =  float3(0.0f,0.0f,0.0f);
    for(int i = 0;i<4;i++){
        posL += weights[i]*mul(vin.PosL,1.0f), gBoneTransforms[vin.BoneIndices[i]].xyz;
        normalL += weights[i]*mul(vin.normalL,1.0f), (float3x3)gBoneTransforms[vin.BoneIndices[i]];
        tangentL += weights[i]*mul(vin.tangentL.xyz,1.0f), (float3x3)gBoneTransforms[vin.BoneIndices[i]];

    }
    
    vin.PosL = posL;
    vin.NormalL = normalL;
    

#endif
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    vout.PosH = mul(posW, gViewProj);
    
    vout.SsaoPosH = mul(posW, gViewProjTex);
	
    //vout.Color = vin.Color;
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    vout.uv = mul(texCoord, matData.gMatTransform).xy;
    
    vout.ShadowPosH = mul(posW, gShadowTransform);
    
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
    
    pin.SsaoPosH /= pin.SsaoPosH.w;
    float ambientAccess = gSsaoMap.Sample(gSamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;
    
    float4 ambient = gAmbientLight * diffuseAlbedo * ambientAccess;

    float3 shadowFactor = 1.0f;
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);
    
    const float shininess = (1.0f - roughtness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, bumpedNormalW, worldPosToEye, shadowFactor);
    
    float4 finalCol = ambient + directLight;
    
    float3 r = reflect(-worldPosToEye, bumpedNormalW);
    float4 reflectionCol = gCubeMap.Sample(gSamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    finalCol.rgb += shininess * fresnelFactor * reflectionCol.rgb;
    
    finalCol.a = diffuseAlbedo.a;
    return finalCol;
}