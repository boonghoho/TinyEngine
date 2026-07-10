# ThirdParty

외부 라이브러리 소스는 이 폴더 아래에 둔다.

## Libraries

- SDL3: `ThirdParty/SDL3`
  - Source: `https://github.com/libsdl-org/SDL.git`
  - Branch: `main`
  - Checked commit: `1bf6279fc722669f3180df5594e56c739c5d0d21`
- Dear ImGui docking: `ThirdParty/imgui`
  - Source: `https://github.com/ocornut/imgui.git`
  - Branch: `docking`
  - Checked commit: `2af6dd9694288e6befe1edb7ce25510911693c22`

## Visual Studio Setup

`TinyEngine/ThirdParty.props`에서 include 경로와 SDL3 library 경로를 연결한다.

- `$(SolutionDir)ThirdParty\SDL3\include`
- `$(SolutionDir)ThirdParty\imgui`
- `$(SolutionDir)ThirdParty\imgui\backends`

`ThirdParty/SDL3/VisualC/SDL.sln`을 열고 TinyEngine과 같은 Configuration과 Platform으로 `SDL` project를 먼저 build해야 한다. SDL test project는 TinyEngine build에 필요하지 않다. `ThirdParty.props`는 `SDL3.lib`를 link하고 build 후 `SDL3.dll`을 TinyEngine 출력 폴더로 복사한다.

Dear ImGui core와 SDL3/DX11 backend source는 `TinyEngine.vcxproj`에서 TinyEngine과 함께 compile한다.
