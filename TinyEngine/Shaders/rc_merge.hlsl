cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float CascadeIndexFloat;
};

Texture2D NearCascadeTexture : register(t0);

Texture2D FarCascadeTexture : register(t1);

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
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
    Output.UV = Positions[VertexId] * float2(0.5, -0.5) + float2(0.5, 0.5);
    return Output;
}

int GetCascadeIndex()
{
    return (int) (CascadeIndexFloat + 0.5);
}

float GetProbeSpacingPixels(int CascadeIndex)
{
    return 4.0 * (float) (1 << CascadeIndex);
}

int GetRaySide(int CascadeIndex)
{
    return 8 * (1 << CascadeIndex);
}

float4 MergeIntervals(float4 NearInterval, float4 FarInterval)
{
    float3 Radiance = NearInterval.rgb + FarInterval.rgb * NearInterval.a;
    float Visibility = NearInterval.a * FarInterval.a;
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

float4 LoadFarCone(uint2 DestinationProbeCoord, uint DestinationDirectionIndex, int DestinationCascadeIndex)
{
    int FarCascadeIndex = DestinationCascadeIndex + 1;
    int FarRaySide = GetRaySide(FarCascadeIndex);
    float FarProbeSpacingPixels = GetProbeSpacingPixels(FarCascadeIndex);
    float DestinationProbeSpacingPixels = GetProbeSpacingPixels(DestinationCascadeIndex);

    float2 DestinationProbeCenterPixels = ((float2) DestinationProbeCoord + 0.5) * DestinationProbeSpacingPixels;
    float2 FarProbeSpace = DestinationProbeCenterPixels / FarProbeSpacingPixels - float2(0.5, 0.5);
    int2 FarBaseProbeCoord = (int2) floor(FarProbeSpace);
    float2 FarProbeRatio = frac(FarProbeSpace);

    uint FarTextureWidth = 0;
    uint FarTextureHeight = 0;
    FarCascadeTexture.GetDimensions(FarTextureWidth, FarTextureHeight);
    uint2 FarProbeCount = uint2(
        FarTextureWidth / FarRaySide,
        FarTextureHeight / FarRaySide
    );

    float4 FarCone = float4(0.0, 0.0, 0.0, 0.0);
    uint FarDirectionBaseIndex = DestinationDirectionIndex * 4;
    
    [unroll]
    for (uint ChildDirection = 0; ChildDirection < 4; ++ChildDirection)
    {
        uint FarDirectionIndex = FarDirectionBaseIndex + ChildDirection;
        uint2 FarDirectionCoord = uint2(FarDirectionIndex % FarRaySide, FarDirectionIndex / FarRaySide);

        // NOTE(ljh): 먼 probe 간격이 더 넓으므로 주변 2 x 2 probe를 bilinear interpolate한다.
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

                uint2 FarTexelCoord = (uint2) FarProbeCoordSigned * FarRaySide + FarDirectionCoord;
                FarCone += LoadCascade(FarCascadeTexture, FarTexelCoord) * (Weight * 0.25);
            }
        }
    }

    return FarCone;
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    int CascadeIndex = GetCascadeIndex();
    int RaySide = GetRaySide(CascadeIndex);

    uint2 TexelCoord = (uint2) Input.Position.xy;
    uint2 DirectionCoord = TexelCoord % RaySide;
    uint2 ProbeCoord = TexelCoord / RaySide;
    uint DirectionIndex = DirectionCoord.x + DirectionCoord.y * RaySide;

    float4 NearInterval = LoadCascade(NearCascadeTexture, TexelCoord);
    float4 FarCone = LoadFarCone(ProbeCoord, DirectionIndex, CascadeIndex);
    
    return MergeIntervals(NearInterval, FarCone);
}
