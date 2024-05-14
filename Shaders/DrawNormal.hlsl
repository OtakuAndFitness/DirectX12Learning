#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

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
    //float4 Color : COLOR;
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
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];
    
    float3 posL = float3(0.0f,0.0f,0.0f);
    float3 normalL = float3(0.0f,0.0f,0.0f);
    float3 tangentL =  float3(0.0f,0.0f,0.0f);
    for(int i = 0;i<4;i++){
        posL += weights[i]*mul(float4(vin.PosL,1.0f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
        normalL += weights[i]*mul(vin.NormalL, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
        tangentL += weights[i]*mul(vin.TangentL.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);

    }
    
    vin.PosL = posL;
    vin.NormalL = normalL;
    vin.TangentL.xyz = tangentL;

#endif
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
    //vout.Color = vin.Color;
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    vout.uv = mul(texCoord, matData.gMatTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[gMaterialDataIndex];
    float4 diffuseAlbedo = matData.gDiffuseAlbedo;
    uint diffuseTexIndex = matData.gDiffuseMapIndex;
    uint normalMapIndex = matData.gNormalMapIndex;
    
    diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gSamAnisotropicWarp, pin.uv);

    pin.NormalW = normalize(pin.NormalW);
    
    float3 normalV = mul(pin.NormalW, (float3x3)gView);
    return float4(normalV, 0.0f);
}