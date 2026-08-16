cbuffer FrameConstants : register(b0)
{
    float2 Resolution;
    float CascadeIndexFloat;
    float ScenePrimitiveCountFloat;
};

struct ScenePrimitive
{
    float4 Shape;

    // NOTE(ljh): rgb = Light2D RGB * Intensity, a = Light면 1, Collider면 0.
    float4 LightData;
};

// TODO(ljh): 복잡한 occluder shape가 필요해지면 SDF 기반 표현을 검토한다.
// NOTE(ljh): 이번 rc 구현은 occluder, light 와 ray의 교점을 직접 계산한다.
StructuredBuffer<ScenePrimitive> ScenePrimitives : register(t0);

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
    return (int)(CascadeIndexFloat + 0.5);
}

float GetProbeSpacingPixels(int CascadeIndex)
{
    return 4.0 * (float)(1 << CascadeIndex);
}

int GetRaySide(int CascadeIndex)
{
    // NOTE(ljh): 반환값은 ray 개수가 아니라 방향 블록 한 변의 길이다.
    // 실제 방향 수는 RaySide^2이며 C0 = 64, C1 = 256, C2 = 1024, C3 = 4096이다.
    return 8 * (1 << CascadeIndex);
}

float IntervalScale(int CascadeIndex)
{
    float Scale = 0.0;

    if (CascadeIndex > 0)
    {
        Scale = (float) (1 << (2 * CascadeIndex));
    }

    return Scale;
}

float2 GetIntervalRangePixels(int CascadeIndex)
{
    const float BaseIntervalLengthPixels = 8.0;

    return BaseIntervalLengthPixels * float2(IntervalScale(CascadeIndex), IntervalScale(CascadeIndex + 1));
}

float IntersectCircle(
    float2 RayOrigin,
    float2 RayDirection,
    float2 Center,
    float Radius,
    float2 IntervalRange)
{
    float2 OriginToCenter = RayOrigin - Center;
    float B = dot(OriginToCenter, RayDirection);
    float C = dot(OriginToCenter, OriginToCenter) - Radius * Radius;
    float Discriminant = B * B - C;
    float HitDistance = -1.0;

    if (Discriminant >= 0.0)
    {
        float Root = sqrt(Discriminant);
        float EnterDistance = -B - Root;
        float ExitDistance = -B + Root;
        float CandidateDistance = max(EnterDistance, IntervalRange.x);

        if (ExitDistance >= CandidateDistance && CandidateDistance <= IntervalRange.y)
        {
            HitDistance = CandidateDistance;
        }
    }

    return HitDistance;
}

bool IntersectSlab(
    float RayOrigin,
    float RayDirection,
    float SlabMinimum,
    float SlabMaximum,
    inout float NearDistance,
    inout float FarDistance)
{
    bool bIntersects = true;

    if (abs(RayDirection) < 0.00001)
    {
        bIntersects = RayOrigin >= SlabMinimum && RayOrigin <= SlabMaximum;
    }
    else
    {
        float InverseDirection = 1.0 / RayDirection;
        float Distance0 = (SlabMinimum - RayOrigin) * InverseDirection;
        float Distance1 = (SlabMaximum - RayOrigin) * InverseDirection;

        NearDistance = max(NearDistance, min(Distance0, Distance1));
        FarDistance = min(FarDistance, max(Distance0, Distance1));
        bIntersects = NearDistance <= FarDistance;
    }

    return bIntersects;
}

float IntersectAabb(
    float2 RayOrigin,
    float2 RayDirection,
    float2 BoxMinimum,
    float2 BoxMaximum,
    float2 IntervalRange)
{
    // NOTE(ljh): Collider를 AABB occluder로 보고 slab 방식으로 ray 교점을 구한다.
    float NearDistance = IntervalRange.x;
    float FarDistance = IntervalRange.y;
    float HitDistance = -1.0;

    bool bIntersects = IntersectSlab(
            RayOrigin.x,
            RayDirection.x,
            BoxMinimum.x,
            BoxMaximum.x,
            NearDistance,
            FarDistance);

    if (bIntersects)
    {
        bIntersects = IntersectSlab(
            RayOrigin.y,
            RayDirection.y,
            BoxMinimum.y,
            BoxMaximum.y,
            NearDistance,
            FarDistance);
    }

    if (bIntersects)
    {
        HitDistance = NearDistance;
    }

    return HitDistance;
}

float4 CastInterval(float2 ProbeCenterPixels, float2 Direction, float2 IntervalRangePixels)
{
    uint PrimitiveCapacity = 0;
    uint PrimitiveStride = 0;
    ScenePrimitives.GetDimensions(PrimitiveCapacity, PrimitiveStride);
    uint PrimitiveCount = min((uint) (ScenePrimitiveCountFloat + 0.5), PrimitiveCapacity);

    float ClosestDistance = IntervalRangePixels.y + 1.0;
    float3 ClosestRadiance = float3(0.0, 0.0, 0.0);
    bool HitAnything = false;

    // NOTE(ljh): ray 하나마다 모든 scene primitive를 순회해 가장 가까운 것을 찾는다.
    [loop]
    for (uint PrimitiveIndex = 0; PrimitiveIndex < PrimitiveCount; ++PrimitiveIndex)
    {
        ScenePrimitive Primitive = ScenePrimitives[PrimitiveIndex];
        bool IsLight = Primitive.LightData.a > 0.5;
        float HitDistance = -1.0;

        if (IsLight)
        {
            HitDistance = IntersectCircle(
                ProbeCenterPixels,
                Direction,
                Primitive.Shape.xy,
                Primitive.Shape.z,
                IntervalRangePixels
            );
        }
        else
        {
            HitDistance = IntersectAabb(
                ProbeCenterPixels,
                Direction,
                Primitive.Shape.xy,
                Primitive.Shape.zw,
                IntervalRangePixels
            );
        }

        if (HitDistance >= 0.0 && HitDistance < ClosestDistance)
        {
            ClosestDistance = HitDistance;
            ClosestRadiance = Primitive.LightData.rgb;
            HitAnything = true;
        }
    }


    return HitAnything ? float4(ClosestRadiance, 0.0) : float4(0.0, 0.0, 0.0, 1.0);
}

float4 PSMain(VSOutput Input) : SV_TARGET
{
    int CascadeIndex = GetCascadeIndex();
    int RaySide = GetRaySide(CascadeIndex);
    int DirectionCount = RaySide * RaySide;

    uint2 TexelCoord = (uint2) Input.Position.xy;
    uint2 DirectionCoord = TexelCoord % RaySide;
    uint2 ProbeCoord = TexelCoord / RaySide;
    
    int DirectionIndex = (int) (DirectionCoord.x + DirectionCoord.y * RaySide);
    float Angle = 6.2831853 * (((float) DirectionIndex + 0.5) / (float) DirectionCount);
    float2 Direction = float2(cos(Angle), sin(Angle));

    float ProbeSpacingPixels = GetProbeSpacingPixels(CascadeIndex);
    float2 ProbeCenterPixels = ((float2) ProbeCoord + 0.5) * ProbeSpacingPixels;
    float2 IntervalRangePixels = GetIntervalRangePixels(CascadeIndex);

    return CastInterval(ProbeCenterPixels, Direction, IntervalRangePixels);
}
