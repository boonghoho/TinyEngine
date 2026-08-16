cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float AmbientStrength;
    float IndirectStrength;
};

Texture2D MergedCascade0Texture : register(t0);

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
    const uint RaySide = 8;
    const uint DirectionCount = RaySide * RaySide;

    ProbeCoord = min(ProbeCoord, ProbeCount - 1);
    float3 Irradiance = float3(0.0, 0.0, 0.0);

    // NOTE(ljh): 한 probe의 64방향 radiance를 평균내 방향성이 없는 irradiance 하나로 만든다.
    [unroll]
    for (uint DirectionIndex = 0; DirectionIndex < DirectionCount; ++DirectionIndex)
    {
        uint2 DirectionCoord = uint2(
            DirectionIndex % RaySide,
            DirectionIndex / RaySide
        );
        uint2 TexelCoord = ProbeCoord * RaySide + DirectionCoord;

        Irradiance += MergedCascade0Texture.Load(int3(TexelCoord, 0)).rgb;
    }

    return Irradiance / (float) DirectionCount;
}

float3 LoadIrradiance(float2 PixelCoord)
{
    const uint RaySide = 8;
    const float ProbeSpacingPixels = 4.0;

    uint Width = 0;
    uint Height = 0;
    MergedCascade0Texture.GetDimensions(Width, Height);

    uint2 ProbeCount = uint2(Width / RaySide, Height / RaySide);
    float2 ProbeSpace = PixelCoord / ProbeSpacingPixels - float2(0.5, 0.5);
    int2 BaseProbeCoord = (int2) floor(ProbeSpace);
    float2 Ratio = frac(ProbeSpace);
    float3 Irradiance = float3(0.0, 0.0, 0.0);

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
            Irradiance += LoadProbeIrradiance((uint2) ProbeCoordSigned, ProbeCount) * Weight;
        }
    }

    return Irradiance;
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    float2 PixelCoord = Input.Uv * Resolution;
    float3 Irradiance = LoadIrradiance(PixelCoord);

    float3 Lighting = max(AmbientStrength, 0.0) + Irradiance * max(IndirectStrength, 0.0);

    return float4(Lighting, 1.0);
}
