# sgl.h

A single-header C99 library for SDL3 that implements a **"Vertex Pulling"** rendering backend.

I wrote this to test the concepts from Mason Ramali's "It's Not About The API" talk. It abstracts the verbose SDL3 GPU API into a simple immediate-mode style API (similar to Raylib) while using modern bindless techniques under the hood.

## Features

- **Single Header:** Drop `sgl.h` into your project.
- **Vertex Pulling:** Uses Storage Buffers (SSBO) instead of Vertex Buffers.
- **Bindless Texture Arrays:** Renders thousands of unique textures in a single draw call using a dynamic 2D Array.
- **Unified 2D/3D Pipeline:** Batches 2D UI and 3D primitives (Cubes) in the same pass using degenerate triangles.
- **Automatic Asset Scaling:** Automatically resizes loaded images to fit the texture array constraints using `SDL_ScaleSurface`.
- **Backend Agnostic:** Works on Vulkan, Metal, D3D12 (handled by SDL3).

## How it works

Instead of constantly binding Vertex Buffers and swapping Textures for every shape, this library uses a modern, data-driven approach:

1. Allocates one massive **Storage Buffer** (for geometry) and one massive **Texture Array** (for bindless images) on the GPU at startup.
2. Maps a pointer to CPU memory every frame using a Transfer Buffer.
3. Writes raw instance data (`x, y, z, color, texture index, etc.`) linearly to that pointer.
4. Uploads the geometry data in one massive batch when `sgl_EndDrawing()` or a camera mode switch is triggered.
5. Pushes calculated Camera View/Projection Matrices via **Uniforms** (Binding 0).
6. Uses **Vertex Pulling** in the Vertex Shader to generate geometry mathematically on the fly using `gl_VertexIndex`. It uses "degenerate triangles" (snapping extra vertices to `0.0`) to seamlessly mix 6-vertex 2D quads and 36-vertex 3D cubes in the exact same draw call.
7. Uses **Bindless-style Texture Fetching** in the Fragment Shader. The shader reads the `texIndex` from the instance data and pulls the exact image slice it needs from the global `sampler2DArray`, completely eliminating CPU-side texture binding overhead.

## SDL3 SPIR-V Resource Mapping

SDL3 expects SPIR-V resources to be bound in specific sets and bindings. When writing your own GLSL shaders for `sgl`, use this table:

| Resource Type | GLSL Keyword | Set (Space) | Binding | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Storage Buffers** | `buffer` | `set = 0` | `binding = 0` | Instance Data (Rects, Colors, Angles). Defined via `SDL_BindGPUVertexStorageBuffers`. |
| **Uniform Buffers** | `uniform` | `set = 1` | `binding = 0` | Global Data (Screen Size, Camera). Defined via `SDL_PushGPUVertexUniformData`. |
| **Textures** | `uniform sampler2DArray` | `set = 2` | `binding = 0` | A single massive array containing up to 256 texture layers. |

## Setup

### The Shaders

You must use GLSL/HLSL shaders that read from a `StructuredBuffer` if you want to use your own shader.
Save these in a folder named `shaders/`.

> [!NOTE]
> **SGL now has embedded default shaders!**
> No longer need to ship or load a `shaders/` folder. The library works out-of-the-box with a single header file.

#### Vertex Shader (`shaders/default.vert`)

Note: This shader now accepts the Screen Size via a Uniform buffer (slot `b0`) so it handles window resizing automatically.

```glsl
// ... [Input Structs] ...

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) flat out int outTexIndex;
layout(location = 3) flat out float outType;

void main() {
    InstanceData inst = instances.data[gl_InstanceIndex];
    int type = int(inst.params2.x);

    // UNIFIED 3D & 2D LOGIC
    if (type == 100) { // CUBE
        // ... [Vertex Pulling for 36-vertex Cube] ...
    } else { // 2D SHAPE
        // ... [Vertex Pulling for 6-vertex Quad] ...
    }
    
    outTexIndex = int(inst.params2.y); // Pass texture ID to Fragment Shader
    gl_Position = mvp * vec4(localPos, 1.0);
}
```

#### Fragment Shader (`shaders/default.frag`)

Save this as `shaders/default.frag`

```glsl
// ... [Inputs] ...

// ONE BINDING TO RULE THEM ALL
layout(set = 2, binding = 0) uniform sampler2DArray globalTextures;

void main() {
    // Bindless Texture Fetching
    if (inTexIndex >= 0) {
        // Z-coordinate selects the layer slice!
        outFragColor = texture(globalTextures, vec3(inUV, float(inTexIndex))) * inColor;
    } else {
        outFragColor = inColor;
    }
}
```

### Compiling Shader

Since the extensions are now .vert and .frag (standard GLSL extensions), the compilation command is slightly simpler.

```bash
glslc shaders/default.vert -o shaders/default.vert.spv
glslc shaders/default.frag -o shaders/default.frag.spv
```

## Implementation

In exactly one C file (e.g., `main.c`), define `SGL_IMPLEMENTATION` before including the header.

```C
#define SGL_IMPLEMENTATION
#include "sgl.h"

int main(int argc, char** argv) {
    sgl_InitWindow(800, 600, "SGL 2D/3D Example");

    SGL_Camera cam2d;
    sgl_CameraInit(&cam2d, 0.0f, 0.0f, 1.0f);

    while (!sgl_WindowShouldClose()) {
        sgl_CameraUpdate(&cam2d);

        sgl_BeginDrawing();

            sgl_BeginMode2D(&cam2d);
                sgl_DrawRectangle(10, 10, 100, 100, (SGL_COLOR){0, 255, 0, 255});
                sgl_DrawTriangle(400, 300, 50, (SGL_COLOR){255, 0, 0, 255});
            sgl_EndMode2D();

        sgl_EndDrawing();
    }

    sgl_Shutdown();
    return 0;
}
```

### Building

Link against SDL3 and the math library

```bash
gcc testbed/main.c -o main -lSDL3 -lm
./main
```

### configuration

You can override defaults by defining these before including the header:

```C
#define SGL_MAX_INSTANCES 50000 // Increase max sprites per frame
#include "sgl.h"
```

### Logs

now `sgl` will reports:

- **Graphics Backend**: (e.g., Vulkan, Metal, Direct3D12). but mainly will outputing vulkan cause of the SPIRV assignment in the gpu create device.

```C
    SDL_GPUDevice *dev =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
```

- **Shader Details**: How many Uniform and Storage Buffers are active per shader.

### Known Issues

- Manual Compilation: Shaders must be pre-compiled to SPIR-V using glslc (from the Vulkan SDK). Runtime GLSL compilation is not currently included in SDL3 core if want to use custom shaders.

- Resource Limits: The default instance limit is 10,000 sprites per frame. Increase SGL_MAX_INSTANCES if you see flickering or missing sprites.

- Coordinate System: The default shader assumes (0,0) is Top-Left. If you port shaders from OpenGL, you may need to flip the Y-axis calculation.

## Roadmap

- [x] **Bindless Textures:** Implemented `sampler2DArray` with a Free-List memory allocator.
- [x] **3D Support:** Implemented `sgl_DrawCube` and 3D Camera support.
- [ ] **Sprite Sheets:** Add `sgl_DrawTexturePart` (Source Rect support).
- [ ] **Text Rendering:** Implement Bitmap Fonts using the new Texture Array system.
