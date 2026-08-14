#include "level.h"

#include "../Renderer/sprite_renderer.h"
#include "../../ThirdParty/nlohmann/json.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

namespace tiny
{

bool Level::Initialize(ID3D11Device* Device, const char* MapFilePath)
{
    if (!Device || !MapFilePath)
    {
        return false;
    }

    std::ifstream MapFile(MapFilePath);
    if (!MapFile)
    {
        std::printf("Failed to open Tiled map: %s\n", MapFilePath);
        return false;
    }

    try
    {
        nlohmann::json MapJson;
        MapFile >> MapJson;

        const nlohmann::json& Tilesets = MapJson.at("tilesets");
        const nlohmann::json& Layers = MapJson.at("layers");

        // NOTE(ljh): 현재는 finite orthogonal map과 embedded tileset 1개만 지원한다.
        if (MapJson.value("orientation", "") != "orthogonal" ||
            MapJson.value("infinite", true) ||
            Tilesets.size() != 1 ||
            Layers.empty())
        {
            std::printf("Unsupported Tiled map layout\n");
            return false;
        }

        const nlohmann::json& Tileset = Tilesets.at(0);
        const nlohmann::json& Layer = Layers.at(0);

        // NOTE(ljh): UV 계산을 단순하게 유지하기 위해 tileset의 margin/spacing은 지원하지 않는다.
        if (Tileset.contains("source") ||
            Tileset.value("tilerendersize", "tile") != "grid" ||
            Tileset.value("margin", 0) != 0 ||
            Tileset.value("spacing", 0) != 0 ||
            Layer.value("type", "") != "tilelayer" ||
            !Layer.at("data").is_array())
        {
            std::printf("Unsupported Tiled tileset or layer\n");
            return false;
        }

        MapWidth = MapJson.at("width").get<i32>();
        MapHeight = MapJson.at("height").get<i32>();
        MapTileWidth = MapJson.at("tilewidth").get<i32>();
        MapTileHeight = MapJson.at("tileheight").get<i32>();

        FirstGID = Tileset.at("firstgid").get<u32>();
        TilesetColumns = Tileset.at("columns").get<i32>();
        TilesetTileWidth = Tileset.at("tilewidth").get<i32>();
        TilesetTileHeight = Tileset.at("tileheight").get<i32>();
        TilesetImageWidth = Tileset.at("imagewidth").get<i32>();
        TilesetImageHeight = Tileset.at("imageheight").get<i32>();
        const i32 TilesetTileCount = Tileset.at("tilecount").get<i32>();

        // NOTE(ljh): Tiled의 tile layer data는 row-major 순서의 GID(Global Tile ID) 배열로 저장된다.
        TileGIDs = Layer.at("data").get<std::vector<u32>>();

        Colliders.clear();

        // NOTE(ljh): Objects layer의 Collider 사각형은 회전 없는 world-space AABB로 저장한다.
        for (const nlohmann::json& CurrentLayer : Layers)
        {
            if (CurrentLayer.value("type", "") != "objectgroup" ||
                CurrentLayer.value("name", "") != "Objects")
            {
                continue;
            }

            const nlohmann::json& Objects = CurrentLayer.at("objects");
            if (!Objects.is_array())
            {
                std::printf("Invalid Tiled object layer\n");
                return false;
            }

            for (const nlohmann::json& Object : Objects)
            {
                if (Object.value("type", "") != "Collider")
                {
                    continue;
                }

                const f32 X = Object.at("x").get<f32>();
                const f32 Y = Object.at("y").get<f32>();
                const f32 Width = Object.at("width").get<f32>();
                const f32 Height = Object.at("height").get<f32>();
                const f32 Rotation = Object.value("rotation", 0.0f);

                const bool bIsUnsupportedShape =
                    Object.contains("ellipse") ||
                    Object.contains("point") ||
                    Object.contains("polygon") ||
                    Object.contains("polyline") ||
                    Object.contains("gid") ||
                    Object.contains("text") ||
                    Object.contains("template") ||
                    Object.contains("capsule");

                if (Width <= 0.0f || Height <= 0.0f || Rotation != 0.0f || bIsUnsupportedShape)
                {
                    std::printf("Unsupported Tiled Collider object\n");
                    return false;
                }

                Colliders.push_back({
                    {X, Y},
                    {X + Width, Y + Height},
                });
            }

            break;
        }

        const std::size_t ExpectedTileCount = static_cast<std::size_t>(MapWidth) * static_cast<std::size_t>(MapHeight);

        if (MapWidth <= 0 || MapHeight <= 0 ||
            MapTileWidth <= 0 || MapTileHeight <= 0 ||
            FirstGID == 0 || TilesetColumns <= 0 || TilesetTileCount <= 0 ||
            TilesetTileWidth <= 0 || TilesetTileHeight <= 0 ||
            TilesetImageWidth <= 0 || TilesetImageHeight <= 0 ||
            Layer.at("width").get<i32>() != MapWidth ||
            Layer.at("height").get<i32>() != MapHeight ||
            TileGIDs.size() != ExpectedTileCount)
        {
            std::printf("Invalid Tiled map dimensions\n");
            return false;
        }

        // NOTE(ljh): Tiled는 GID의 상위 bit에 flip/rotation 정보를 저장한다.
        // 현재는 tile transform을 지원하지 않는다.
        constexpr u32 TransformFlagMask = 0xF0000000u;
        for (u32 GID : TileGIDs)
        {
            if ((GID & TransformFlagMask) != 0 ||
                (GID != 0 && (GID < FirstGID || GID - FirstGID >= static_cast<u32>(TilesetTileCount))))
            {
                std::printf("Unsupported Tiled tile GID\n");
                return false;
            }
        }

        const std::filesystem::path JsonPath(MapFilePath);
        const std::filesystem::path ImagePath = (JsonPath.parent_path() / std::filesystem::path(Tileset.at("image").get<std::string>())).lexically_normal();

        const std::u8string ImagePathUtf8 = ImagePath.u8string();
        const std::string ImagePathString(reinterpret_cast<const char*>(ImagePathUtf8.data()), ImagePathUtf8.size());

        if (!TilesetTexture.LoadFromFile(Device, ImagePathString.c_str()) ||
            TilesetTexture.GetWidth() != TilesetImageWidth ||
            TilesetTexture.GetHeight() != TilesetImageHeight)
        {
            std::printf("Failed to load Tiled tileset image: %s\n", ImagePathString.c_str());
            return false;
        }

        std::printf(
            "Loaded Tiled map: %s (%dx%d tiles, %zu colliders)\n",
            MapFilePath,
            MapWidth,
            MapHeight,
            Colliders.size());

        return true;
    }
    catch (const std::exception& Error)
    {
        std::printf("Failed to parse Tiled map: %s\n", Error.what());
        TileGIDs.clear();
        Colliders.clear();

        return false;
    }
}

void Level::RenderTileMap(SpriteRenderer& Renderer) const
{
    if (TileGIDs.empty())
    {
        return;
    }

    for (i32 TileY = 0; TileY < MapHeight; ++TileY)
    {
        for (i32 TileX = 0; TileX < MapWidth; ++TileX)
        {
            // NOTE(ljh): 2D tile 좌표를 row-major 1D 배열 index로 변환한다.
            const std::size_t TileIndex = static_cast<std::size_t>(TileY * MapWidth + TileX);

            const u32 GID = TileGIDs[TileIndex];

            // NOTE(ljh): Tiled에서 GID 0은 빈 tile을 의미한다.
            if (GID == 0)
            {
                continue;
            }

            // NOTE(ljh): Global Tile ID를 현재 tileset 내부의 local tile ID로 변환한다.
            const u32 LocalTileID = GID - FirstGID;

            const i32 SourceColumn = static_cast<i32>(LocalTileID) % TilesetColumns;
            const i32 SourceRow = static_cast<i32>(LocalTileID) / TilesetColumns;

            // NOTE(ljh): tileset image의 pixel 영역을 shader에서 사용하는 0~1 범위의 UV 좌표로 변환한다.
            const UVRect SourceUV = {
                static_cast<f32>(SourceColumn * TilesetTileWidth) / TilesetImageWidth,
                static_cast<f32>(SourceRow * TilesetTileHeight) / TilesetImageHeight,
                static_cast<f32>((SourceColumn + 1) * TilesetTileWidth) / TilesetImageWidth,
                static_cast<f32>((SourceRow + 1) * TilesetTileHeight) / TilesetImageHeight,
            };

            Renderer.Draw(
                TilesetTexture.GetShaderResourceView(),
                static_cast<f32>(TileX * MapTileWidth),
                static_cast<f32>(TileY * MapTileHeight),
                static_cast<f32>(MapTileWidth),
                static_cast<f32>(MapTileHeight),
                SourceUV);
        }
    }
}

Entity& Level::CreateEntity()
{
    Entities.push_back({NextEntityID++});
    return Entities.back();
}

Entity* Level::GetEntityByID(u32 ID)
{
    for (Entity& CurrentEntity : Entities)
    {
        if (CurrentEntity.ID == ID)
        {
            return &CurrentEntity;
        }
    }

    return nullptr;
}

}
