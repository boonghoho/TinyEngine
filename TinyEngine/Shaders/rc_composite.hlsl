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
Texture2D MergedCascade0Texture : register(t1);
SamplerState SceneSampler : register(s0);

float GetIndirectStrength()
{
    return max(Param0, 0.0);
}

float GetExposure()
{
    return max(Param1, 0.001);
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

float3 LoadProbeIrradiance(uint2 ProbeCoord, uint2 ProbeCount)
{
    const uint RaySide = 4;
    const uint DirCount = RaySide * RaySide;

    ProbeCoord = min(ProbeCoord, ProbeCount - 1);

    float3 Irradiance = float3(0.0, 0.0, 0.0);

    // NOTE(ljh): C0 probe가 모든 방향에서 모은 빛을 평균낸다.
    [loop]
    for (uint DirIndex = 0; DirIndex < DirCount; ++DirIndex)
    {
        uint2 DirCoord = uint2(DirIndex % RaySide, DirIndex / RaySide);
        uint2 TexelCoord = ProbeCoord * RaySide + DirCoord;
        Irradiance += MergedCascade0Texture.Load(int3(TexelCoord, 0)).rgb;
    }

    return Irradiance / (float)DirCount;
}

float3 LoadIrradianceFromCascade0(float2 PixelCoord)
{
    const uint RaySide = 4;
    const float ProbeSpacingPixels = 16.0;

    uint Width = 0;
    uint Height = 0;
    MergedCascade0Texture.GetDimensions(Width, Height);

    uint2 ProbeCount = uint2(Width / RaySide, Height / RaySide);
    float2 ProbeSpace = PixelCoord / ProbeSpacingPixels - float2(0.5, 0.5);
    int2 BaseProbeCoord = (int2)floor(ProbeSpace);
    float2 Ratio = frac(ProbeSpace);

    float3 Irradiance = float3(0.0, 0.0, 0.0);

    // NOTE(ljh): 주변 probe 4개를 bilinear 보간해 격자 형태를 줄인다.
    [unroll]
    for (int OffsetY = 0; OffsetY < 2; ++OffsetY)
    {
        [unroll]
        for (int OffsetX = 0; OffsetX < 2; ++OffsetX)
        {
            int2 ProbeCoordSigned = BaseProbeCoord + int2(OffsetX, OffsetY);
            ProbeCoordSigned = clamp(ProbeCoordSigned, int2(0, 0), int2(ProbeCount) - int2(1, 1));

            float WeightX = OffsetX == 0 ? 1.0 - Ratio.x : Ratio.x;
            float WeightY = OffsetY == 0 ? 1.0 - Ratio.y : Ratio.y;
            float Weight = WeightX * WeightY;

            Irradiance += LoadProbeIrradiance((uint2)ProbeCoordSigned, ProbeCount) * Weight;
        }
    }

    return Irradiance;
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    float3 SceneColorSrgb = SceneTexture.Sample(SceneSampler, Input.Uv).rgb;
    float3 SceneColor = pow(saturate(SceneColorSrgb), 2.2);
    float2 PixelCoord = Input.Uv * Resolution;
    float3 Irradiance = LoadIrradianceFromCascade0(PixelCoord);

    float3 DirectEmission = SceneColor * 1.15;
    float3 IndirectLight = Irradiance * GetIndirectStrength();
    float3 Color = DirectEmission + IndirectLight;

    // NOTE(ljh): Tone mapping으로 밝은 색이 흰색으로 뭉치는 것을 줄인다.
    Color = 1.0 - exp(-Color * GetExposure());
    Color = pow(saturate(Color), 1.0 / 2.2);

    return float4(Color, 1.0);
}
