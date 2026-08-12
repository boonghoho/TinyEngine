#pragma once

#include "entity.h"
#include "../Renderer/texture2d.h"

#include <vector>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace tiny
{

class SpriteRenderer;

class Level
{
public:
    bool Initialize(ID3D11Device* Device, const char* MapFilePath);
    void RenderTileMap(ID3D11DeviceContext* DeviceContext, SpriteRenderer& Renderer) const;

    Entity& CreateEntity();
    Entity* GetEntityByID(u32 ID);

    std::vector<Entity>& GetEntities() { return Entities; }

private:
    Texture2D TilesetTexture;
    std::vector<u32> TileGIDs;

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

    std::vector<Entity> Entities;
    u32 NextEntityID = 1;
};

}
