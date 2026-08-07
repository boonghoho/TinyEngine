#pragma once

#include <d3d11.h>

// NOTE(ljh): HLSL 파일의 Entry Point를 컴파일하여 D3D11 Shader bytecode를 만든다.
namespace tiny
{

bool CompileShaderFromFile(
    const wchar_t* FilePath,
    const char* EntryPoint,
    const char* TargetProfile,
    ID3DBlob** OutBlob
);

}
