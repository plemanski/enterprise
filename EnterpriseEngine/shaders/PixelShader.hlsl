struct PixelShaderInput
{
    float4 PositionVS  : POSITION;
    float3 NormalVS    : NORMAL;
    float2 TexCoord    : TEXCOORD;
};

Texture2D Texture                   : register(t0);
SamplerState LinearRepeatSampler    : register(s0);

float4 main ( PixelShaderInput IN) : SV_Target
{
    return Texture.Sample(LinearRepeatSampler, IN.TexCoord);
}