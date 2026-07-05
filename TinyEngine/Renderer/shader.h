#pragma once

#include <d3d11.h>

bool CompileShaderFromFile(
    const wchar_t* FilePath,
    const char* EntryPoint,
    const char* TargetProfile,
    ID3DBlob** OutBlob
);
