#pragma once

#include <SDL3/SDL_events.h>

#include <array>
#include <cstddef>

namespace tiny
{

class Input
{
public:
    void BeginFrame();
    void ProcessEvent(const SDL_Event& Event);

    bool IsDown(SDL_Scancode Key) const;
    bool WasPressed(SDL_Scancode Key) const;
    bool WasReleased(SDL_Scancode Key) const;

private:
    static constexpr std::size_t KeyCount = SDL_SCANCODE_COUNT;

    static bool IsValidKey(SDL_Scancode Key);
    void ClearKeyboardState();

    std::array<bool, KeyCount> DownKeys{};
    std::array<bool, KeyCount> PressedKeys{};
    std::array<bool, KeyCount> ReleasedKeys{};
};

}
