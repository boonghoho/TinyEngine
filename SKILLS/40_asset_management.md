# 40 — 에셋 관리

## 현재 구현

### 이미지/텍스처 로딩

```
stb_image로 PNG 로드 → SDL_Surface 변환 → SDL_Texture 생성
```

- `stbi_load()`: 파일에서 raw 픽셀 데이터 로드 (RGBA 4채널 강제)
- `SDL_CreateSurfaceFrom()`: raw 데이터 → SDL_Surface
- `SDL_CreateTextureFromSurface()`: SDL_Surface → GPU 텍스처
- 로드 후 CPU측 데이터(`stbi_image_free`) 및 Surface 즉시 해제

### 에셋 경로

- 현재: 실행 파일 기준 상대 경로 (`"test.png"`)
- `assets/` 디렉토리가 존재하나 아직 빌드 파이프라인에서 복사하지 않음

> TODO: `build.zig`에서 `assets/` 디렉토리를 `zig-out/bin/`으로 복사하는 Install 스텝 추가 필요.

## Vulkan 전환 시 에셋 파이프라인 (TODO)

### 텍스처 업로드 흐름 (예상)

```
stb_image → Staging Buffer (CPU visible) → vkCmdCopyBufferToImage → Image (GPU local)
```

1. `stbi_load()`로 이미지 데이터 로드
2. Staging Buffer 생성 (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
3. 데이터 복사 → Staging Buffer
4. VkImage 생성 (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
5. Layout Transition: `UNDEFINED` → `TRANSFER_DST_OPTIMAL`
6. `vkCmdCopyBufferToImage`
7. Layout Transition: `TRANSFER_DST_OPTIMAL` → `SHADER_READ_ONLY_OPTIMAL`
8. VkImageView + VkSampler 생성
9. Staging Buffer 해제

> 이 흐름은 [TA] 모드로 단계별로 학습하며 구현한다.

## 2D 스프라이트 배칭 (TODO)

- 동일 텍스처의 스프라이트를 하나의 Draw Call로 묶는 배칭 시스템
- 텍스처 아틀라스(Texture Atlas) 지원 고려
- Vertex Buffer에 위치/UV/색상을 모아서 한 번에 제출

## 3D 모델 로딩 (Phase 3, TODO)

> 아이소메트릭 3D 전환 시 필요. 현재 범위 밖.
> - 포맷: TODO (glTF 2.0 유력)
> - 로더: TODO (tinygltf, cgltf 등 검토 필요)
