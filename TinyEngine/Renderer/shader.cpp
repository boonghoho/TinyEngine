#include "shader.h"

#include <d3dcompiler.h>
#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")

namespace tiny
{

bool CompileShaderFromFile(
    const wchar_t* FilePath,
    const char* EntryPoint,
    const char* TargetProfile,
    ID3DBlob** OutBlob
)
{
    ID3DBlob* ErrorBlob = nullptr;

    HRESULT Result = D3DCompileFromFile(
        FilePath,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        EntryPoint,
        TargetProfile,
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        OutBlob,
        &ErrorBlob
    );

    if (ErrorBlob)
    {
        std::printf("%s\n", static_cast<const char*>(ErrorBlob->GetBufferPointer()));
        ErrorBlob->Release();
    }

    if (FAILED(Result))
    {
        std::printf("D3DCompileFromFile failed for %s (%s): 0x%08X\n", EntryPoint, TargetProfile, Result);
        return false;
    }

    return true;
}

}
