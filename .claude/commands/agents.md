Purpose

Initialize a custom C++/Vulkan/SDL3 game engine (TinyEngine) repository so that AI coding agents (Claude Code, Cursor, etc.) can operate safely, deterministically, and with minimal human supervision, while strictly adhering to a Socratic Teaching Assistant (TA) methodology for core graphics engine tasks.

Inputs

- Repository to initialize.
- Optional: short human note describing the project and instructions.

Outputs

- AGENTS.md at repository root (Agent operational contract & Mode definitions)
- SKILLS/ directory with structured TinyEngine technical documentation

Core Principles

- Do NOT guess. If information is missing, explicitly mark TODOs.
- Prefer correctness, memory safety, and Vulkan validation layer compliance over completeness.
- Never modify compiled binaries (.spv, .exe, .a, .so) or raw media assets (.png, .gltf, .ttf). Only interact with C++ source, Zig build scripts (build.zig), Shader source (.vert, .frag, .comp), Configs, and Markdown.
- Respect modern C++ paradigms and the Zig build system's strictness regarding undefined behavior.
- Optimize for agent execution, not human prose.

Execution Steps

Step 1: Repository Inspection (TinyEngine-Specific)

Detect:
- Build System (Zig build system, `build.zig`).
- Vulkan SDK version and wrapper usage (e.g., raw vulkan.h vs Vulkan-Hpp).
- SDL3 integration (Window creation, Input handling, Vulkan surface extension).
- Math Library (e.g., GLM) and GUI Library (e.g., Dear ImGui).

Identify:
- C++ Source structure (e.g., src/, include/, vendor/).
- Shader asset pipeline (Where GLSL/HLSL files are stored and how they are compiled via Zig build steps or external scripts).
- Core Engine vs. Game Logic separation.
- Perspective handling (2D orthographic vs 3D Isometric/Quarter-view camera logic).

Step 2: Confirm Ambiguities With the Human

Request the user to clarify:
- The architectural pattern for game objects (e.g., pure ECS, DOD, or traditional OOP).
- Vulkan memory management strategies (e.g., VMA - Vulkan Memory Allocator usage vs custom).
- How C++ dependencies (SDL3, Vulkan headers) are linked within `build.zig` (e.g., via system packages, submodules, or Zig package manager).

Step 3: Generate AGENTS.md (Crucial)

Copy the standard AGENTS structure, then heavily edit it based on inspection.
AGENTS.md MUST include:

1. Agent Operation & Mode Rules (Strict enforcement):
   - Define [TA] Mode (Teaching Assistant): Default mode. For core C++ architecture, Vulkan rendering pipelines, memory synchronization (Fences/Semaphores), custom ECS/DOD optimization, and shader logic. The agent MUST NOT provide drop-in code. Use the Socratic method, provide pseudocode, Khronos Vulkan Spec links, and ask leading questions about GPU memory layout and cache coherence.
   - Define [DO] Mode (Task Delegation): Triggered by user tag. For SDL3 window/input boilerplate, `build.zig` modifications, adding new C++ files to the Zig build pipeline, simple math utility functions, struct data binding, and repetitive shader compilation steps. The agent provides complete, copy-pasteable, efficient code without unnecessary explanation.
2. C++ & Vulkan Coding Style:
   - Enforce specific naming conventions (e.g., Vulkan handles, custom wrapper prefixes, struct naming).
   - Enforce strict Resource Acquisition Is Initialization (RAII) for Vulkan object destruction to prevent memory leaks.
3. Build & Run Procedures:
   - Explicitly list the Zig CLI commands required (e.g., `zig build`, `zig build run`, or specific steps like `zig build shaders`).
4. Language Policy:
   - All documentation, comments, and commit messages must be in Korean.

Step 4: Generate SKILLS/ Structure (Custom Engine Focused)

Use `SKILLS/` as the target directory. Create the following files, omitting clearly irrelevant ones:
- SKILLS/00_overview.md (Engine architecture, 2D/Isometric 3D scope, Main Game Loop with SDL3)
- SKILLS/10_vulkan_pipeline.md (Swapchain setup, RenderPass/Dynamic Rendering, Descriptor Sets, Pipeline layout)
- SKILLS/20_entity_logic.md (ECS or object update loops, Transform hierarchy for Isometric projection)
- SKILLS/30_build_and_shaders.md (Zig `build.zig` structure, C++ compilation flags, shader compilation automation via Zig)
- SKILLS/40_asset_management.md (Texture loading, Sprite batching for 2D, 3D model loading, Buffer uploading)
- SKILLS/50_memory_and_sync.md (Vulkan allocation strategies, Command Buffer recording, Sync primitives)
- SKILLS/80_style_guide.md (C++ standard used, .clang-format rules, struct padding rules)

Rules:
- Never fabricate external dependencies unless explicitly found in `build.zig` or source includes.
- Prefer explicit "Unknown / TODO" over assumptions regarding custom engine math or physics.

Step 5: Validation Checklist

Before finalizing:
- All `zig build` commands and Shader compile scripts are copy-paste runnable OR marked TODO.
- No assumptions made about generic game engine features.
- The distinction between [TA] (Vulkan/Core Arch) and [DO] (SDL3/Zig Boilerplate) tasks is clearly articulated in AGENTS.md.

Failure Handling

If the repository is missing a `build.zig` file or a main C++ source directory:
- Pause and ask the user if this is indeed the TinyEngine project directory. Do not proceed with initialization if false.