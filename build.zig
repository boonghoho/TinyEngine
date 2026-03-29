const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // 1. 실행 파일 설정
    const exe = b.addExecutable(.{
        .name = "TinyEngine",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    // glm 추가
    exe.addIncludePath(b.path("vendor/glm"));

    // stb image
    exe.addIncludePath(b.path("vendor/stb"));

    // imgui 추가
    exe.addIncludePath(b.path("vendor/imgui"));

    const imgui_files = &[_][]const u8{
        "vendor/imgui/imgui.cpp",
        "vendor/imgui/imgui_demo.cpp",
        "vendor/imgui/imgui_draw.cpp",
        "vendor/imgui/imgui_tables.cpp",
        "vendor/imgui/imgui_widgets.cpp",
        // 중요: SDL3용 백엔드 소스 (imgui 폴더 안 backends에 있음)
        "vendor/imgui/backends/imgui_impl_sdl3.cpp",
        "vendor/imgui/backends/imgui_impl_sdlrenderer3.cpp",
    };

    // ImGui 소스 파일들을 C++ 컴파일 목록에 추가
    for (imgui_files) |file| {
        exe.addCSourceFile(.{
            .file = b.path(file),
            .flags = &[_][]const u8{ "-std=c++20" }, // C++ 버전 맞춤
        });
    }

    // 2. C++ 소스 추가
    exe.addCSourceFile(.{
        .file = b.path("src/main.cpp"),
        .flags = &[_][]const u8{ "-std=c++20" },
    });

    // 3. SDL3 연결 설정
    
    // (1) 헤더 파일 경로 (.h)
    exe.addIncludePath(b.path("vendor/SDL3/include"));

    // (2) 라이브러리 파일 경로 (.lib)
    exe.addLibraryPath(b.path("vendor/SDL3/lib/x64"));
    
    // (3) SDL3 라이브러리 링크
    exe.linkSystemLibrary("SDL3"); // SDL3.lib를 찾아서 링크함
    
    // --------------------------------------------------

    exe.linkLibCpp();
    exe.linkLibC();

    b.installArtifact(exe);

    // 4. 실행 시 DLL 복사
    const install_dll = b.addInstallFile(
        b.path("vendor/SDL3/lib/x64/SDL3.dll"),
        "bin/SDL3.dll"
    );
    b.getInstallStep().dependOn(&install_dll.step);

    // 5. 실행 명령어 설정
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    
    const run_step = b.step("run", "Run the engine");
    run_step.dependOn(&run_cmd.step);
}