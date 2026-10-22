#pragma once

#include "../Core/math_types.h"
#include "entity.h"
#include "../Renderer/texture2d.h"

#include <vector>

struct ID3D11Device;

namespace tiny
{

struct Camera2D;
struct RenderSettings;
struct RenderFrameData;
struct SpriteDrawCommand;

class Level
{
public:
    bool Initialize(ID3D11Device* Device, const char* MapFilePath);
    void BuildRenderFrame(const Camera2D& Camera, const RenderSettings& Settings, RenderFrameData& Frame) const;

    Entity& CreateEntity();
    Entity* GetEntityByID(u32 ID);

    const std::vector<Entity>& GetEntities() const { return Entities; }
    const std::vector<AABB>& GetColliders() const { return Colliders; }

private:
    void AppendTileSprites(std::vector<SpriteDrawCommand>& Sprites) const;

    struct TileLayer
    {
        std::vector<u32> TileGIDs;
        Light2D Light;
        bool bIsVisible = true;
        bool bCreatesLights = false;
    };

    Texture2D TilesetTexture;
    std::vector<TileLayer> TileLayers;

    i32 MapWidth = 0;
    i32 MapHeight = 0;
    i32 MapTileWidth = 0;
    i32 MapTileHeight = 0;

    u32 FirstGID = 0;
    i32 TilesetColumns = 0;
    i32 TilesetTileWidth = 0;
    i32 TilesetTileHeight = 0;
    i32 TilesetImageWidth = 0;
    i32 TilesetImageHeight = 0;

    std::vector<AABB> Colliders;
    std::vector<Entity> Entities;
    u32 NextEntityID = 1;
};

}
