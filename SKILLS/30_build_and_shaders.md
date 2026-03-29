# 30 — 빌드 시스템 및 셰이더 파이프라인

## Zig 빌드 시스템

### 기본 명령어

```bash
# 전체 빌드
zig build

# 빌드 + 실행
zig build run
```

### build.zig 구조 (현재)

```
1. 실행 파일 생성 (TinyEngine)
2. C++ 소스 컴파일 (-std=c++20)
   - src/main.cpp
   - vendor/imgui/*.cpp (ImGui 코어 + SDL3/SDL_Renderer3 백엔드)
3. Include 경로 설정
   - vendor/glm
   - vendor/stb
   - vendor/imgui
   - vendor/SDL3/include
4. 라이브러리 링크
   - SDL3.lib (vendor/SDL3/lib/x64)
   - libc++, libc
5. DLL 복사 (SDL3.dll → zig-out/bin/)
6. Run 스텝 정의
```

### 새 C++ 파일 추가 방법 ([DO] 모드)

`build.zig`에 `addCSourceFile` 호출을 추가한다:

```zig
exe.addCSourceFile(.{
    .file = b.path("src/새파일.cpp"),
    .flags = &[_][]const u8{ "-std=c++20" },
});
```

Include 경로가 필요하면 `addIncludePath`도 추가:

```zig
exe.addIncludePath(b.path("include"));
```

### 의존성 링크 방식

- **Zig 패키지 매니저 미사용** (`build.zig.zon`의 dependencies는 비어 있음)
- 모든 의존성은 `vendor/` 디렉토리에 직접 포함
- SDL3: 사전 빌드된 `.lib` + `.dll` (Windows x64)
- GLM, stb, ImGui: 헤더/소스 직접 포함

### 컴파일 플래그

- C++ 표준: `-std=c++20`
- 추가 플래그: TODO (경고 수준, 최적화 등 필요 시 추가)

## 셰이더 컴파일 파이프라인

> **상태**: 미구현. Vulkan 전환 시 구축 필요.

### 예상 구조 (TODO)

```
shaders/
├── sprite.vert       # GLSL 버텍스 셰이더
├── sprite.frag       # GLSL 프래그먼트 셰이더
└── compiled/
    ├── sprite.vert.spv   # 컴파일된 SPIR-V (gitignore 대상)
    └── sprite.frag.spv
```

### build.zig 셰이더 컴파일 스텝 (TODO)

```zig
// TODO: Vulkan 전환 시 아래와 같은 빌드 스텝 추가
// const shader_compile = b.addSystemCommand(&.{
//     "glslc",
//     "shaders/sprite.vert",
//     "-o", "shaders/compiled/sprite.vert.spv",
// });
// exe.step.dependOn(&shader_compile.step);
```

> `glslc` (Google/Shaderc) 또는 `glslangValidator` (Khronos) 중 선택 필요.
> Zig의 `addSystemCommand`로 빌드 파이프라인에 통합한다.
