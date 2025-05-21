struct PixelShaderInput
{
    float4 PositionVS  : POSITION;
    float3 NormalVS    : NORMAL;
    float2 TexCoord    : TEXCOORD;
	float4 Position	   : SV_POSITION;
};

struct Light
{
	float4 PositionWS;
	float4 PositionVS;
	float4 Colour;
	float4 DirectionWS;
	float4 DirectionVS;
	float4 CameraWS;
	float  AmbientStrength;
	float  SpecularStrength;
};

struct LightResult
{
	float4 Diffuse;
	float4 Specular;
	float4 Ambient;
};


Texture2D Texture                   : register(t0);
StructuredBuffer<Light> Lights		: register(t1);
SamplerState LinearRepeatSampler    : register(s0);

float DoDiffuse( float3 dir, float3 normal)
{
	return max(0, dot(dir, normal));
};

float DoSpecular( float3 lightDir, float3 normal, float3 viewDir, float specStrength)
{
	float3 reflectDir = normalize(reflect(-lightDir, normal));
	// Eventually get the power from the material
	float specComponent = pow(max(dot(viewDir, reflectDir),0),32);
	return (specStrength * specComponent);
};

LightResult DoLight(Light light, float3 surfacePos, float3 surfaceNorm, float3 viewPos)
{
	// SurfacePos is in viewspace, inverse will give vector to point from eye
	float3 viewVector = normalize(viewPos - surfacePos);
	LightResult result;
	float3 LVector = (light.PositionVS.xyz - surfacePos);
	float lengthL = length(LVector);
	LVector = LVector/lengthL;
	result.Diffuse = DoDiffuse(LVector, surfaceNorm) * light.Colour;
	result.Specular = DoSpecular(LVector, surfaceNorm, viewVector, light.SpecularStrength);
	result.Ambient = light.AmbientStrength;
    return result;
};

float4 main ( PixelShaderInput IN) : SV_Target
{
	LightResult light = DoLight(Lights[0], IN.PositionVS.xyz, normalize(IN.NormalVS.xyz), IN.Position.xyz);
    float4 texColour = Texture.Sample(LinearRepeatSampler, IN.TexCoord);
	return (light.Ambient + light.Diffuse + light.Specular) * texColour;
};