struct VSInput
{
    float2 Position : POSITION;
    float2 Uv : TEXCOORD0;
    float4 Color : COLOR0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 Uv : TEXCOORD0;
    float4 Color : COLOR0;
};

cbuffer CameraConstants : register(b0)
{
    row_major float4x4 WorldToClip;
};

Texture2D SpriteTexture : register(t0);
SamplerState SpriteSampler : register(s0);

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;

    // NOTE(ljh): TinyEngine은 행 벡터 규칙을 사용한다: Vector * Matrix.
    Output.Position = mul(float4(Input.Position, 0.0f, 1.0f), WorldToClip);
    Output.Uv = Input.Uv;
    Output.Color = Input.Color;
    return Output;
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    return SpriteTexture.Sample(SpriteSampler, Input.Uv) * Input.Color;
}
