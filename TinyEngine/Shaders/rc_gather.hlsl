cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float2 Mouse;
    float Time;
    float CascadeIndexFloat;
    float SceneWidth;
    float SceneHeight;
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

int GetCascadeIndex()
{
    return (int)(CascadeIndexFloat + 0.5);
}

float GetProbeSpacingPixels(int CascadeIndex)
{
    return 16.0 * (float)(1 << CascadeIndex);
}

int GetRaySide(int CascadeIndex)
{
    return 4 * (1 << CascadeIndex);
}

float IntervalScale(int CascadeIndex)
{
    if (CascadeIndex <= 0)
    {
        return 0.0;
    }

    return (float)(1 << (2 * CascadeIndex));
}

float2 GetIntervalRangePixels(int CascadeIndex)
{
    const float BaseIntervalLengthPixels = 8.0;
    return BaseIntervalLengthPixels * float2(
        IntervalScale(CascadeIndex),
        IntervalScale(CascadeIndex + 1)
    );
}

bool IsScenePixel(float3 Color)
{
    return dot(Color, Color) > 0.001;
}

float4 CastInterval(float2 ProbeCenterPixels, float2 Direction, float2 IntervalRangePixels)
{
    const float StepPixels = 4.0;
    float IntervalLengthPixels = max(IntervalRangePixels.y - IntervalRangePixels.x, StepPixels);
    int StepCount = max(1, (int)ceil(IntervalLengthPixels / StepPixels));

    // NOTE(ljh): RGB에는 처음 만난 빛의 색을 저장한다.
    // NOTE(ljh): Alpha에는 빛이 더 멀리 갈 수 있는지를 저장한다.
    [loop]
    for (int StepIndex = 0; StepIndex < StepCount; ++StepIndex)
    {
        float StepT = ((float)StepIndex + 0.5) / (float)StepCount;
        float DistancePixels = lerp(IntervalRangePixels.x, IntervalRangePixels.y, StepT);
        float2 SamplePixels = ProbeCenterPixels + Direction * DistancePixels;
        float2 SampleUv = SamplePixels / float2(SceneWidth, SceneHeight);

        if (SampleUv.x < 0.0 || SampleUv.x > 1.0 || SampleUv.y < 0.0 || SampleUv.y > 1.0)
        {
            continue;
        }

        float3 SceneColorSrgb = SceneTexture.SampleLevel(SceneSampler, SampleUv, 0.0).rgb;

        if (IsScenePixel(SceneColorSrgb))
        {
            float3 SceneColorLinear = pow(saturate(SceneColorSrgb), 2.2);
            return float4(SceneColorLinear, 0.0);
        }
    }

    return float4(0.0, 0.0, 0.0, 1.0);
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    int CascadeIndex = GetCascadeIndex();
    int RaySide = GetRaySide(CascadeIndex);
    int DirCount = RaySide * RaySide;

    // NOTE(ljh): 각 probe는 RaySide x RaySide 영역을 사용하며 texel 하나가 ray 방향 하나를 나타낸다.
    uint2 TexelCoord = (uint2)Input.Position.xy;
    uint2 DirCoord = TexelCoord % RaySide;
    uint2 ProbeCoord = TexelCoord / RaySide;

    int DirIndex = (int)(DirCoord.x + DirCoord.y * RaySide);
    float Angle = 6.2831853 * (((float)DirIndex + 0.5) / (float)DirCount);
    float2 Direction = float2(cos(Angle), sin(Angle));

    float ProbeSpacingPixels = GetProbeSpacingPixels(CascadeIndex);
    float2 ProbeCenterPixels = ((float2)ProbeCoord + 0.5) * ProbeSpacingPixels;
    float2 IntervalRangePixels = GetIntervalRangePixels(CascadeIndex);

    return CastInterval(ProbeCenterPixels, Direction, IntervalRangePixels);
}
