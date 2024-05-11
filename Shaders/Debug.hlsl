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
       
    vout.PosH = float4(vin.PosL,1.0f);
	
    vout.uv = vin.TexCoord;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(gSsaoMap.Sample(gSamLinearWrap, pin.uv).rrr, 1.0f);
}