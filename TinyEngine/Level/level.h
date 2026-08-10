#pragma once

#include "entity.h"
#include <vector>

namespace tiny
{

class Level
{
public:
    void Initialize();
    Entity& CreateEntity();
	Entity* GetEntityByID(u32 ID);

	std::vector<Entity>& GetEntities() { return Entities; }

private:
    std::vector<Entity> Entities;
    u32 NextEntityID = 1;
};

}
