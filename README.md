# TinyEngine

작은 2D·2.5D·탑뷰 3D 게임을 직접 만들면서 엔진 구조와 현대적인 렌더링 기술을 공부하는 C++ 프로젝트입니다.

<img width="1274" height="741" alt="image" src="https://github.com/user-attachments/assets/666c6f53-de05-4c2b-84e9-a938a85731e9" />

## 목표

- 작은 게임을 직접 만들 수 있을 정도로 단순하고 이해하기 쉬운 엔진을 만든다.
- 창, 입력, 시간, 씬, 에셋, 에디터와 렌더러를 직접 구현하며 구조를 익힌다.
- 우선 2D에 집중하고 나중에 2.5D와 탑뷰 3D로 넓힌다.
- DirectX 11로 시작해 Vulkan, DirectX 12 순서로 확장한다.

## 구현된 기능

- SDL3 창 생성, 키보드 입력과 게임 루프
- DirectX 11 디바이스, 스왑 체인과 렌더 파이프라인
- Texture2D, 스프라이트 렌더러와 동적 스프라이트 배칭
- Entity, Transform2D, Sprite, Light2D
- Tiled JSON 맵 로딩과 타일맵 렌더링
- 캐릭터 이동과 AABB 충돌
- FP16 HDR 렌더 텍스처와 톤 매핑
- 움직이는 광원과 차폐물을 반영하는 Radiance Cascades
- Dear ImGui 디버그 UI와 RenderDoc GPU 이벤트 마커
- D3D11 타임스탬프 쿼리를 이용한 GPU 패스 측정
- Release 빌드와 패키징 스크립트

## 다음 목표

- AssetHandle, 에셋 레지스트리, 텍스처 캐시와 폴백 에셋
- 씬 저장·불러오기와 안정적인 Entity 생성·삭제
- Camera2D와 월드·화면 좌표 변환
- 에디터 창 크기와 분리된 게임 뷰포트
- Hierarchy와 Inspector 패널을 포함한 간단한 에디터
- 렌더 레이어, 정렬과 카메라 컬링
- Material2D와 블렌드 모드
- 로그, Assertion과 에디터 콘솔
- 셰이더 핫 리로드
- 에셋 로딩에 적용할 작은 Job System 실험
- 다른 Graphics API를 붙일 수 있도록 렌더러 의존성 정리
