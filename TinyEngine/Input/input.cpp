#include "input.h"

namespace tiny
{

void Input::BeginFrame()
{
    PressedKeys.fill(false);
    ReleasedKeys.fill(false);
}

void Input::ProcessEvent(const SDL_Event& Event)
{
    if (Event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
    {
        ClearKeyboardState();
        return;
    }

    if (Event.type != SDL_EVENT_KEY_DOWN && Event.type != SDL_EVENT_KEY_UP)
    {
        return;
    }

    const SDL_Scancode Key = Event.key.scancode;
    if (!IsValidKey(Key))
    {
        return;
    }

    const std::size_t KeyIndex = static_cast<std::size_t>(Key);

    if (Event.type == SDL_EVENT_KEY_DOWN)
    {
        if (!Event.key.repeat && !DownKeys[KeyIndex])
        {
            PressedKeys[KeyIndex] = true;
        }

        DownKeys[KeyIndex] = true;
        return;
    }

    if (DownKeys[KeyIndex])
    {
        ReleasedKeys[KeyIndex] = true;
    }

    DownKeys[KeyIndex] = false;
}

bool Input::IsDown(SDL_Scancode Key) const
{
    if (!IsValidKey(Key))
    {
        return false;
    }

    return DownKeys[static_cast<std::size_t>(Key)];
}

bool Input::WasPressed(SDL_Scancode Key) const
{
    if (!IsValidKey(Key))
    {
        return false;
    }

    return PressedKeys[static_cast<std::size_t>(Key)];
}

bool Input::WasReleased(SDL_Scancode Key) const
{
    if (!IsValidKey(Key))
    {
        return false;
    }

    return ReleasedKeys[static_cast<std::size_t>(Key)];
}

bool Input::IsValidKey(SDL_Scancode Key)
{
    const std::size_t KeyIndex = static_cast<std::size_t>(Key);
    return KeyIndex < KeyCount;
}

void Input::ClearKeyboardState()
{
    DownKeys.fill(false);
    PressedKeys.fill(false);
    ReleasedKeys.fill(false);
}

}
