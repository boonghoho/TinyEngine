cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float Param0;
};

Texture2D SpriteColorTexture : register(t0);
Texture2D LightTexture : register(t1);
SamplerState SceneSampler : register(s0);

float GetExposure()
{
    return max(Param0, 0.001);
}

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 Uv : TEXCOORD0;
};

VSOutput VSMain(uint VertexId : SV_VertexID)
{
    float2 Positions[3] =
    {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0)
    };

    VSOutput Output;
    Output.Position = float4(Positions[VertexId], 0.0, 1.0);
    Output.Uv = Positions[VertexId] * float2(0.5, -0.5) + float2(0.5, 0.5);
    return Output;
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    float4 SpriteColorSrgb = SpriteColorTexture.Sample(SceneSampler, Input.Uv);
    float3 SceneColor = pow(saturate(SpriteColorSrgb.rgb), 2.2);
    float3 Lighting = max(LightTexture.Sample(SceneSampler, Input.Uv).rgb, 0.0);
    
    float3 Color = SceneColor * Lighting;

    Color = 1.0 - exp(-Color * GetExposure());
    Color = pow(saturate(Color), 1.0 / 2.2);

    return float4(Color, SpriteColorSrgb.a);
}
