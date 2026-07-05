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
- zigimg: `ThirdParty/zigimg`
  - Source: `https://github.com/zigimg/zigimg.git`
  - Branch: `master`
  - Checked commit: `4cab5a17d4c76584723576fcc11497293c25d092`

## Visual Studio Setup

`TinyEngine/ThirdParty.props`에서 include 경로만 연결한다.

- `$(SolutionDir)ThirdParty\SDL3\include`
- `$(SolutionDir)ThirdParty\imgui`
- `$(SolutionDir)ThirdParty\imgui\backends`

SDL3 빌드 방식, 라이브러리 링크, ImGui 백엔드 연결 코드는 아직 추가하지 않는다. 이 프로젝트는 학습용이므로 실제 초기화와 통합 구현은 사용자가 직접 진행한다.
