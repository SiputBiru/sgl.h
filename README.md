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

<br>
## Setup

### The Shaders

You must use GLSL/HLSL shaders that read from a `StructuredBuffer` if you want to use your own shader.
Save these in a folder named `shaders/`.

> [!NOTE]
> **SGL now has embedded default shaders!**
> No longer need to ship or load a `shaders/` folder. The library works out-of-the-box with a single header file.

### Custom Shaders

You can write your own custom GLSL shaders to create special effects (like glowing outlines, CRT scanlines, or custom 3D lighting).
If you write a custom shader, it must conform to the library's bindless memory layout.

#### Custom Vertex Shader (`custom.vert`)

Your vertex shader must read from the `std430` instance storage buffer (Set 0) and apply the Camera matrix (Set 1). You generate the geometry mathematically using `gl_VertexIndex`.

```glsl
#version 450

// --- SET 0: The SGL Instance Data ---
struct InstanceData {
    vec4 rect;    // x, y, z, size
    vec4 params;  // angle, ox, oy, unused
    vec4 params2; // type, texIndex, p2, p3
    vec4 color;   // r, g, b, a
};

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    InstanceData data[];
} instances;

// --- SET 1: The Camera Matrix ---
layout(set = 1, binding = 0) uniform Uniforms {
    mat4 mvp; // Model-View-Projection matrix from sgl_BeginMode
};

// Outputs to your Fragment Shader
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) flat out int outTexIndex;

void main() {
    InstanceData inst = instances.data[gl_InstanceIndex];
    
    // ... [Your custom Vertex Pulling logic goes here] ...
    // Calculate localPos and outUV based on gl_VertexIndex

    outColor = inst.color;
    outTexIndex = int(inst.params2.y);
    gl_Position = mvp * vec4(localPos, 1.0);
}
```

#### Custom Fragment Shader (custom.frag)

If your custom shader needs to draw images, it must access SGL's global texture array (Set 2).

```glsl
#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) flat in int inTexIndex;

// --- SET 2: The SGL Texture Array ---
layout(set = 2, binding = 0) uniform sampler2DArray globalTextures;

layout(location = 0) out vec4 outFragColor;

void main() {
    vec4 baseColor = inColor;

    // Apply texture if the instance has one
    if (inTexIndex >= 0) {
        baseColor *= texture(globalTextures, vec3(inUV.x, inUV.y, float(inTexIndex)));
    }

    // ... [Your custom pixel effects (e.g., color tinting, glowing, discarding) go here] ...

    outFragColor = baseColor;
}
```

#### Compiling and loading

Custom shaders must be pre-compiled to SPIR-V using glslc (included in the Vulkan SDK).

```bash
glslc shaders/custom.vert -o shaders/custom.vert.spv
glslc shaders/custom.frag -o shaders/custom.frag.spv
```

then, load and apply them in your C code using SGL's pipeline API:

```C
// Load the compiled SPIR-V files
// Signature: sgl_LoadShader(filename, stage, num_uniforms, num_storage, num_samplers)
SDL_GPUShader* customVert = sgl_LoadShader("shaders/custom.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1, 1, 0);
SDL_GPUShader* customFrag = sgl_LoadShader("shaders/custom.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 1);

// Create the pipeline
SDL_GPUGraphicsPipeline* customPipeline = sgl_CreatePipeline(customVert, customFrag);

while (!sgl_WindowShouldClose()) {
    sgl_BeginDrawing();
    
    // Set your custom pipeline
    sgl_SetPipeline(customPipeline);
    
    // Draw shapes with your special effects!
    sgl_DrawRectangle(10, 10, 100, 100, (SGL_COLOR){255, 0, 0, 255});
    
    // Reset back to SGL's default pipeline for normal drawing
    sgl_SetPipeline(NULL); 
    sgl_DrawRectangle(150, 10, 100, 100, (SGL_COLOR){0, 255, 0, 255});
    
    sgl_EndDrawing();
}
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
