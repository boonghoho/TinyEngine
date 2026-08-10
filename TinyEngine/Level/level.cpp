#include "level.h"

namespace tiny
{

void Level::Initialize()
{
	// TODO(ljh): tiled json 을 통해 현재 Level의 맵 타일을 불러와 배치한다.
}

Entity& Level::CreateEntity()
{
    Entities.push_back({ NextEntityID++ });

    return Entities.back();
}

Entity* Level::GetEntityByID(u32 ID)
{
    for (Entity& entity : Entities)
    {
        if (entity.ID == ID)
        {
            return &entity;
        }
	}

    return nullptr;
}

}
