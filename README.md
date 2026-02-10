# sgl.h

A single-header C99 library for SDL3 that implements a "Vertex Pulling" rendering backend.

I wrote this to test the concepts from Mason Ramali's "It's Not About The API" talk. It abstracts the verbose SDL3 GPU API into a simple immediate-mode style API (similar to Raylib) but uses modern bindless techniques under the hood.

## How it works

Instead of binding vertex buffers for every shape, this library:

1. Allocates one massive Storage Buffer (SSBO) on the GPU at startup.

2. Maps a pointer to CPU memory every frame.

3. Writes raw instance data (x, y, color, etc.) linearly to that pointer.

4. Uploads everything in one go at the end of the frame.

5. Uses a Vertex Pulling shader to generate geometry on the fly using `SV_VertexID` and `SV_InstanceID`.

## Dependencies

- **SDL3** (Latest Nightly or built from source). The GPU API is still in flux, so older versions might break.

- **Vulkan SDK** (specifically `glslc`) to compile the shaders.

## Setup

### 1. The Shaders

You cannot use standard vertex shaders with this library because it does not send vertex attributes. You must use the following HLSL shaders that read from a StructuredBuffer.

Save these in a folder named `shaders/`.

#### Vertex Shader (`shaders/shader.vert.hlsl`)

```High-level shader language
struct InstanceData {
float4 rect; // x, y, w, h
float4 color; // r, g, b, a
// padding handled in C struct
};

StructuredBuffer<InstanceData> Instances : register(t0, space0);

struct VSInput {
uint vertexID : SV_VertexID;
uint instanceID : SV_InstanceID;
};

struct VSOutput {
float4 pos : SV_Position;
float4 color : TEXCOORD0;
};

VSOutput main(VSInput input) {
VSOutput output;

    // Pull Data
    InstanceData data = Instances[input.instanceID];

    // Generate Quad (0..5)
    float2 corner;
    uint idx = input.vertexID % 6;
    if (idx == 0) corner = float2(0, 0);
    else if (idx == 1) corner = float2(0, 1);
    else if (idx == 2) corner = float2(1, 0);
    else if (idx == 3) corner = float2(1, 0);
    else if (idx == 4) corner = float2(0, 1);
    else                 corner = float2(1, 1);

    // World Position
    float2 worldPos = data.rect.xy + (corner * data.rect.zw);

    // NDC Conversion (Assuming 800x600 window)
    float2 ndc;
    ndc.x = (worldPos.x / 800.0) * 2.0 - 1.0;
    ndc.y = -((worldPos.y / 600.0) * 2.0 - 1.0);

    output.pos = float4(ndc, 0.0, 1.0);
    output.color = data.color;
    return output;

}

```

#### Fragment Shader (`shaders/shader.frag.hlsl`)

```High-level shader language
float4 main(float4 pos : SV_Position, float4 color : TEXCOORD0) : SV_Target0 {
    return color;
}
```

**Compiling Shaders** Run these commands in your terminal to generate the SPIR-V bytecode:

```Bash
glslc -fshader-stage=vertex shaders/shader.vert.hlsl -o shaders/shader.vert.spv
glslc -fshader-stage=fragment shaders/shader.frag.hlsl -o shaders/shader.frag.spv
```

## 2. Implementation

In exactly one C file (e.g., `main.c`), define the implementation before including the header.

```C
#define SGL_IMPLEMENTATION
#include "sgl.h"
```

### Example Usage

```C
#include <SDL3/SDL.h>
#define SGL_IMPLEMENTATION
#include "sgl.h"

int main(int argc, char* argv[]) {
SDL_Init(SDL_INIT_VIDEO);
SDL_Window* window = SDL_CreateWindow("SGL Demo", 800, 600, 0);

    // Create GPU Device (Vulkan/SPIR-V backend)
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

    // Initialize Library (Loads shaders from disk automatically)
    if (!sgl_Init(window, device)) {
        return -1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        if (cmd) {
            sgl_Begin(cmd);

            // Draw things (Linear allocation, very fast)
            sgl_DrawRectangle(100, 100, 200, 200, (SGL_COLOR){255, 0, 0, 255});
            sgl_DrawRectangle(350, 100, 200, 200, (SGL_COLOR){0, 255, 0, 255});

            sgl_End(window);
            SDL_SubmitGPUCommandBuffer(cmd);
        }
    }

    sgl_Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

## Building

Link against SDL3 and the math library.

```Bash
gcc testbed/main.c -o demo -lSDL3 -lm
./demo
```

## Configuration

You can override the maximum instance count or window size assumptions by defining these before including the header:

```C
#define SGL_MAX_INSTANCES 50000
#define SGL_WIN_WIDTH 1920
#define SGL_WIN_HEIGHT 1080
#include "sgl.h"
```

## Known Issues

The projection matrix is currently hardcoded to 800x600 in the shader.

Only supports SPIR-V (Vulkan) right now. If you want D3D12/Metal, you need to compile the shaders to DXIL/MSL and change the `SDL_CreateGPUDevice` flags.
