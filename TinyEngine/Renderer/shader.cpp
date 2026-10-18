#include "shader.h"

#include "../Core/log.h"

#include <d3dcompiler.h>

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

    UINT CompileFlags = 0;

#if defined(_DEBUG)
    CompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    CompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT Result = D3DCompileFromFile(
        FilePath,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        EntryPoint,
        TargetProfile,
        CompileFlags,
        0,
        OutBlob,
        &ErrorBlob
    );

    if (ErrorBlob)
    {
        if (FAILED(Result))
        {
            TE_LOG_ERROR("Shader", "%s", static_cast<const char*>(ErrorBlob->GetBufferPointer()));
        }
        else
        {
            TE_LOG_WARNING("Shader", "%s", static_cast<const char*>(ErrorBlob->GetBufferPointer()));
        }
        ErrorBlob->Release();
    }

    if (FAILED(Result))
    {
        TE_LOG_ERROR("Shader", "D3DCompileFromFile failed for %s (%s): 0x%08X\n", EntryPoint, TargetProfile, Result);
        return false;
    }

    return true;
}

}
