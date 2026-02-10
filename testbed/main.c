#define SDL_MAIN_USE_CALLBACKS 0

#include <SDL3/SDL.h>

#define SGL_IMPLEMENTATION
#include "../sgl.h"

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    SDL_Window *window =
        SDL_CreateWindow("SGL - SDL3 GPU Batcher", 800, 600, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUDevice *device =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!device) {
        SDL_Log("GPU Device failed: %s", SDL_GetError());
        return -1;
    }

    if (!sgl_Init(window, device)) {
        SDL_Log("SGL Init failed!");
        return -1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        if (cmd) {
            sgl_Begin(cmd);

            for (int i = 0; i < 800; i += 40) {
                for (int j = 0; j < 600; j += 40) {
                    sgl_DrawRectangle(i, j, 38, 38,
                                      (SGL_COLOR){30, 30, 30, 255});
                }
            }

            f32 time = SDL_GetTicks() / 1000.0f;
            f32 x = 400 + SDL_cosf(time) * 150;
            f32 y = 300 + SDL_cosf(time) * 150;

            sgl_DrawRectangle(0, 550, 800, 50, (SGL_COLOR){50, 50, 200, 255});
            sgl_DrawRectangle(10, 560, 200, 30,
                              (SGL_COLOR){200, 200, 200, 255});

            sgl_End(window);

            SDL_SubmitGPUCommandBuffer(cmd);
        }
    }

    sgl_Shutdown();
    // SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
