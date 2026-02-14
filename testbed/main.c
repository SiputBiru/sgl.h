#define SGL_IMPLEMENTATION
#include "../sgl.h"

int main(int argc, char **argv) {
    // Init (Will try to load default.vert.spv / default.frag.spv internally)
    sgl_InitWindow(800, 600, "SGL testbed Default & Custom");

    // OPTIONAL: Load a custom shader (e.g., a shader that makes everything
    // Red) You only do this if you want to override the default.
    /*
    SDL_GPUShader* myVS = sgl_LoadShader("custom.vert.spv",
    SDL_GPU_SHADERSTAGE_VERTEX, 1, 1); SDL_GPUShader* myFS =
    sgl_LoadShader("custom.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    SDL_GPUGraphicsPipeline* myPipeline = sgl_CreatePipeline(myVS, myFS);
    */

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_EVENT_QUIT) {

                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_Q) {
                    running = false;
                }
            }

        sgl_Begin();

        // -----------------------------------------------------
        // Scenario A: Use Default Shader (No setup needed)
        // -----------------------------------------------------
        sgl_SetPipeline(sgl_GetDefaultPipeline()); // Or just pass NULL
        sgl_DrawRectangle(10, 10, 100, 100,
                          (SGL_Color){0, 255, 0, 255}); // Green

        // -----------------------------------------------------
        // Scenario B: Use Custom Shader (If you loaded one)
        // -----------------------------------------------------
        /*
        if (myPipeline) {
             // Flush the previous batch before switching shaders?
             // (Note: Currently sgl only supports 1 pipeline per frame in this
        simple version.
             //  Real batching would require a 'Flush' function here.)

             sgl_SetPipeline(myPipeline);
             sgl_DrawRectangle(200, 10, 100, 100, (SGL_Color){255, 0, 0, 255});
        }
        */

        sgl_End();
    }

    sgl_Shutdown();
    return 0;
}
