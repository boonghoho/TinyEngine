cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float2 Mouse;
    float Time;
    float3 Padding;
};

Texture2D SceneTexture : register(t0);
SamplerState SceneSampler : register(s0);

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 Uv : TEXCOORD0;
};

// vertex shader
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


bool IsScenePixel(float3 Color)
{
    return dot(Color, Color) > 0.001;
}

float3 Raymarch(float2 StartUv, float2 Direction)
{
    const int MaxSteps = 96;
    const float StepPixels = 8.0;
    const float MaxDistancePixels = MaxSteps * StepPixels;

    float2 StartPixels = StartUv * Resolution;

    [loop]
    for (int StepIndex = 1; StepIndex < MaxSteps; ++StepIndex)
    {
        float DistancePixels = StepIndex * StepPixels;
        float2 SamplePixels = StartPixels + Direction * DistancePixels;
        float2 SampleUv = SamplePixels / Resolution;

        if (SampleUv.x < 0.0 || SampleUv.x > 1.0 || SampleUv.y < 0.0 || SampleUv.y > 1.0)
        {
            break;
        }

        float3 SceneColor = SceneTexture.SampleLevel(SceneSampler, SampleUv, 0.0).rgb;

        if (IsScenePixel(SceneColor))
        {
            float Attenuation = 1.0 - saturate(DistancePixels / MaxDistancePixels);
            return SceneColor * Attenuation;
        }
    }

    return float3(0.0, 0.0, 0.0);
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    float3 SceneColor = SceneTexture.Sample(SceneSampler, Input.Uv).rgb;

    if (IsScenePixel(SceneColor))
    {
        return float4(SceneColor, 1.0);
    }

    const int RayCount = 32;
    float3 Radiance = float3(0.0, 0.0, 0.0);

    [loop]
    for (int RayIndex = 0; RayIndex < RayCount; ++RayIndex)
    {
        float Angle = ((float)RayIndex / (float)RayCount) * 6.2831853;
        float2 Direction = float2(cos(Angle), sin(Angle));

        Radiance += Raymarch(Input.Uv, Direction);
    }

    Radiance /= (float)RayCount;

    return float4(Radiance, 1.0);
}
