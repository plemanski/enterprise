struct PixelShaderInput
{
    float4 PositionVS  : POSITION;
    float3 NormalVS    : NORMAL;
    float2 TexCoord    : TEXCOORD;
};

struct Light
{
	float4 PositionWS;
	float4 PositionVS;
	float4 Colour;
	float4 DirectionWS;
	float4 DirectionVS;
	float4 Padding;
	float4 Padding1;
	float4 Padding2;
};

struct LightResult
{
	float4 Diffuse;
	float4 Specular;
};


Texture2D Texture                   : register(t0);
StructuredBuffer<Light> Lights		: register(t1);
SamplerState LinearRepeatSampler    : register(s0);

// Still needs colour applied
float DoDiffuse( float3 dir, float3 normal)
{
	return max(0, dot(dir, normal));
};

LightResult DoLight(Light light, float3 surfacePos, float3 surfaceNorm)
{
	// SurfacePos is in viewspace, inverse will give vector to point from eye
	float3 viewVector = normalize( -surfacePos);
	LightResult result;
	float3 LVector = (light.PositionVS.xyz - surfacePos);
	float lengthL = length(LVector);
	LVector = LVector/lengthL;
	result.Diffuse = DoDiffuse(LVector, surfaceNorm) * light.Colour;
	result.Specular = 0;
    return result;
};

float4 main ( PixelShaderInput IN) : SV_Target
{
	LightResult light = DoLight(Lights[0], IN.PositionVS.xyz, normalize(IN.NormalVS.xyz));
    float4 texColour = Texture.Sample(LinearRepeatSampler, IN.TexCoord);
	return light.Diffuse * texColour;
};