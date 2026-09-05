#include "camera2d.h"

namespace tiny
{

Mat4 Camera2D::BuildWorldToClip() const
{
    Mat4 Result = {};

    const f32 ScaleX = 2.0f * Zoom / ViewportWidth;
    const f32 ScaleY = 2.0f * Zoom / ViewportHeight;

    // 행 벡터 규칙: [x y 0 1] * WorldToClip
    // 화면 좌표의 Y축은 아래로 증가하므로 Clip Space Y축을 반전한다.
    Result.M[0][0] = ScaleX;
    Result.M[1][1] = -ScaleY;
    Result.M[2][2] = 1.0f;
    Result.M[3][0] = -ScaleX * Position.X;
    Result.M[3][1] = ScaleY * Position.Y;
    Result.M[3][3] = 1.0f;

    return Result;
}

}
