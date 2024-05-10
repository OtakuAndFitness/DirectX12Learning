#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    MaterialData matData = gMaterialData[gMaterialDataIndex];
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    vout.PosH = mul(posW, gViewProj);
	
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    vout.uv = mul(texCoord, matData.gMatTransform).xy;
        
    return vout;
}

void PS(VertexOut pin)
{
    
}