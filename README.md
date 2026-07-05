# TinyEngine

TinyEngine is a learning-oriented C++ engine project for small 2D, 2.5D, and top-view games.

Current focus:

- SDL3 window creation
- DirectX 11 device, swap chain, and fullscreen rendering
- CPU-authored scene texture uploaded to the GPU
- ImGui brush controls
- Flatland Radiance Cascades study prototype

The current prototype lets the user paint emissive geometry into a `SceneTexture` and display it through a fullscreen HLSL pass. The next milestone is baseline raymarching over that scene texture.
