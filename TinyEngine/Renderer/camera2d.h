#pragma once

#include "../Core/math_types.h"

namespace tiny
{

struct Camera2D
{
    // 월드 공간에서 바라보는 화면 중심 위치.
    Vec2 Position;

    // 기본 배율을 1.0f
    f32 Zoom = 1.0f;

    // 카메라가 렌더링할 뷰포트 크기.
    f32 ViewportWidth = 0.0f;
    f32 ViewportHeight = 0.0f;

    // NOTE(ljh): 카메라 상태를 SpriteRenderer가 사용할 World -> Clip 행렬로 변환한다.
    Mat4 BuildWorldToClip() const;
};

}
