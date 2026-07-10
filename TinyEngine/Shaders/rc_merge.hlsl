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

Texture2D FarCascadeTexture : register(t0);
Texture2D SceneTexture : register(t1);
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

float2 GetDirection(uint2 DirCoord, int RaySide)
{
    int DirIndex = (int)(DirCoord.x + DirCoord.y * RaySide);
    int DirCount = RaySide * RaySide;
    float Angle = 6.2831853 * (((float)DirIndex + 0.5) / (float)DirCount);
    return float2(cos(Angle), sin(Angle));
}

bool IsScenePixel(float3 Color)
{
    return dot(Color, Color) > 0.001;
}

float4 CastSceneSegment(float2 StartPixels, float2 EndPixels)
{
    const float StepPixels = 4.0;
    float2 Segment = EndPixels - StartPixels;
    float SegmentLengthPixels = length(Segment);
    int StepCount = max(1, (int)ceil(SegmentLengthPixels / StepPixels));

    [loop]
    for (int StepIndex = 0; StepIndex < StepCount; ++StepIndex)
    {
        float StepT = ((float)StepIndex + 0.5) / (float)StepCount;
        float2 SamplePixels = lerp(StartPixels, EndPixels, StepT);
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

float4 MergeIntervals(float4 Near, float4 Far)
{
    // NOTE(ljh): Near 구간이 막히면 Alpha가 0이 되어 Far 구간의 빛을 차단한다.
    float3 Radiance = Near.rgb + Far.rgb * Near.a;
    float Visibility = Near.a * Far.a;
    return float4(Radiance, Visibility);
}

float4 LoadCascade(Texture2D CascadeTexture, uint2 TexelCoord)
{
    uint Width = 0;
    uint Height = 0;
    CascadeTexture.GetDimensions(Width, Height);
    TexelCoord = min(TexelCoord, uint2(Width - 1, Height - 1));
    return CascadeTexture.Load(int3(TexelCoord, 0));
}

float4 LoadFarCone(uint2 DestProbeCoord, uint2 DestDirCoord, int DestCascadeIndex)
{
    int FarCascadeIndex = DestCascadeIndex + 1;
    int DestRaySide = GetRaySide(DestCascadeIndex);
    int FarRaySide = GetRaySide(FarCascadeIndex);
    float FarProbeSpacingPixels = GetProbeSpacingPixels(FarCascadeIndex);
    float DestProbeSpacingPixels = GetProbeSpacingPixels(DestCascadeIndex);

    float2 DestProbeCenterPixels = ((float2)DestProbeCoord + 0.5) * DestProbeSpacingPixels;
    float2 FarProbeSpace = DestProbeCenterPixels / FarProbeSpacingPixels - float2(0.5, 0.5);
    int2 FarBaseProbeCoord = (int2)floor(FarProbeSpace);
    float2 FarProbeRatio = frac(FarProbeSpace);
    uint2 FarDirBaseCoord = DestDirCoord * 2;

    uint FarTextureWidth = 0;
    uint FarTextureHeight = 0;
    FarCascadeTexture.GetDimensions(FarTextureWidth, FarTextureHeight);
    uint2 FarProbeCount = uint2(FarTextureWidth / FarRaySide, FarTextureHeight / FarRaySide);

    float2 DestDirection = GetDirection(DestDirCoord, DestRaySide);
    float2 DestIntervalRangePixels = GetIntervalRangePixels(DestCascadeIndex);
    float2 DestIntervalStartPixels = DestProbeCenterPixels + DestDirection * DestIntervalRangePixels.x;
    float FarIntervalStartPixels = GetIntervalRangePixels(FarCascadeIndex).x;

    float4 Cone = float4(0.0, 0.0, 0.0, 0.0);

    // NOTE(ljh): 보간할 Far probe까지의 구간을 검사해 벽 너머의 빛 누출을 막는다.
    [unroll]
    for (int OffsetY = 0; OffsetY < 2; ++OffsetY)
    {
        [unroll]
        for (int OffsetX = 0; OffsetX < 2; ++OffsetX)
        {
            int2 FarProbeCoordSigned = FarBaseProbeCoord + int2(OffsetX, OffsetY);
            FarProbeCoordSigned = clamp(
                FarProbeCoordSigned,
                int2(0, 0),
                int2(FarProbeCount) - int2(1, 1)
            );

            float WeightX = OffsetX == 0 ? 1.0 - FarProbeRatio.x : FarProbeRatio.x;
            float WeightY = OffsetY == 0 ? 1.0 - FarProbeRatio.y : FarProbeRatio.y;
            float Weight = WeightX * WeightY;

            uint2 FarProbeCoord = (uint2)FarProbeCoordSigned;
            float2 FarProbeCenterPixels = ((float2)FarProbeCoord + 0.5) * FarProbeSpacingPixels;
            float2 FarConeStartPosition =
                FarProbeCenterPixels + DestDirection * FarIntervalStartPixels;
            float4 ConnectingInterval = CastSceneSegment(
                DestIntervalStartPixels,
                FarConeStartPosition
            );

            float4 ProbeCone = float4(0.0, 0.0, 0.0, 0.0);

            [unroll]
            for (uint ChildY = 0; ChildY < 2; ++ChildY)
            {
                [unroll]
                for (uint ChildX = 0; ChildX < 2; ++ChildX)
                {
                    uint2 FarDirCoord = FarDirBaseCoord + uint2(ChildX, ChildY);
                    uint2 FarTexelCoord = FarProbeCoord * FarRaySide + FarDirCoord;
                    float4 FarInterval = LoadCascade(FarCascadeTexture, FarTexelCoord);
                    ProbeCone += MergeIntervals(ConnectingInterval, FarInterval) * 0.25;
                }
            }

            Cone += ProbeCone * Weight;
        }
    }

    return Cone;
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    int CascadeIndex = GetCascadeIndex();
    int RaySide = GetRaySide(CascadeIndex);

    // NOTE(ljh): Merge 결과도 Near cascade와 같은 probe-direction 배열로 저장한다.
    uint2 TexelCoord = (uint2)Input.Position.xy;
    uint2 DirCoord = TexelCoord % RaySide;
    uint2 ProbeCoord = TexelCoord / RaySide;

    return LoadFarCone(ProbeCoord, DirCoord, CascadeIndex);
}
