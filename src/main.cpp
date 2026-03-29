#include <SDL3/SDL.h>
#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char* argv[]) {
    // 1. SDL �ʱ�ȭ
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("TinyEngine + NanoBanana", 1280, 720, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 2. ImGui �ʱ�ȭ
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // ---------------------------------------------------------
    // [2] �̹��� �ε� �� �ؽ�ó ���� (���� ���� ���� �� 1ȸ ����)
    // ---------------------------------------------------------
    int img_w, img_h, img_channels;
    // test.png�� �ε��մϴ�. (4�� RGBA ä�� ����)
    unsigned char* img_data = stbi_load("assets/test.png", &img_w, &img_h, &img_channels, 4);

    SDL_Texture* bananaTexture = nullptr;

    if (img_data) {
        // Raw Data -> SDL Surface ��ȯ
        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            img_w, img_h,
            SDL_PIXELFORMAT_RGBA32,
            img_data,
            img_w * 4 // Pitch (�� ���� ����Ʈ ũ��)
        );

        // Surface -> GPU Texture ��ȯ
        if (surface) {
            bananaTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface); // �ؽ�ó ��������� ���ǽ��� ����
        }
        else {
            std::cerr << "Surface ���� ����: " << SDL_GetError() << std::endl;
        }

        stbi_image_free(img_data); // ���� �����͵� ����
    }
    else {
        std::cerr << "�̹��� �ε� ����! (��� Ȯ�� �ʿ�): " << stbi_failure_reason() << std::endl;
    }
    // ---------------------------------------------------------

    bool isRunning = true;
    SDL_Event event;

    // ĳ���� ��ġ ���� (ImGui�� �����غ�����!)
    float x = 400.0f;
    float y = 200.0f;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
        }

        // 3. ImGui ������ �غ�
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 4. ImGui â ���� (ĳ���� ��ġ ���� ��� �߰�)
        ImGui::Begin("Tiny Engine Control");
        ImGui::Text("Hello, Nano Banana!");
        ImGui::SliderFloat("Position X", &x, 0.0f, 1280.0f);
        ImGui::SliderFloat("Position Y", &y, 0.0f, 720.0f);
        ImGui::Text("Image Size: %d x %d", img_w, img_h);
        ImGui::End();

        // 5. ������ ���� (���� �߿�!)

        // (1) ȭ�� ����� (������)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // (2) ���� ������Ʈ(�̹���) �׸���
        if (bananaTexture) {
            // �̹����� �׷��� ��ġ�� ũ�� ����
            SDL_FRect destRect = { x, y, (float)img_w, (float)img_h };
            SDL_RenderTexture(renderer, bananaTexture, NULL, &destRect);
        }

        // (3) ImGui �׸��� (���� ���� UI�� ���� �ϹǷ� ���߿� �׸�)
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        // (4) ȭ�� ����
        SDL_RenderPresent(renderer);
    }

    // ���� ó��
    if (bananaTexture) SDL_DestroyTexture(bananaTexture);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}