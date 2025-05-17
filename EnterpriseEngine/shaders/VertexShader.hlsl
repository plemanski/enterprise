struct Transforms
{
    matrix ModelMatrix;
    matrix ModelViewMatrix;
    matrix InverseTransposeModelViewMatrix;
    matrix ModelViewProjectionMatrix;
};

ConstantBuffer<Transforms> TransformsCB : register(b0);

struct VertexPositionNormalTexture
{
    float3 Position    : POSITION;
    float3 Normal      : NORMAL;
    float2 TexCoord    : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionVS  : POSITION;
    float3 NormalVS    : NORMAL;
    float2 TexCoord    : TEXCOORD;
    float4 Position    : SV_Position;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul( TransformsCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
    OUT.PositionVS = mul( TransformsCB.ModelViewMatrix, float4(IN.Position, 1.0f));
    OUT.NormalVS = mul((float3x3)TransformsCB.InverseTransposeModelViewMatrix, IN.Normal);
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}