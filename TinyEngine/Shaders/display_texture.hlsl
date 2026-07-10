cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float2 Mouse;
    float Time;
    float Param0;
    float Param1;
    float Param2;
    float Param3;
    float3 Padding;
};

Texture2D SceneTexture : register(t0);
SamplerState SceneSampler : register(s0);

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
    float3 Color = SceneTexture.Sample(SceneSampler, Input.Uv).rgb;
    return float4(Color, 1.0);
}
