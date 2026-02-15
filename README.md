# sgl.h

A single-header C99 library for SDL3 that implements a **"Vertex Pulling"** rendering backend.

I wrote this to test the concepts from Mason Ramali's "It's Not About The API" talk. It abstracts the verbose SDL3 GPU API into a simple immediate-mode style API (similar to Raylib) while using modern bindless techniques under the hood.

## Features

- **Single Header:** Drop `sgl.h` into your project.
- **Vertex Pulling:** Uses Storage Buffers (SSBO) instead of Vertex Buffers.
- **Backend Agnostic:** Works on Vulkan, Metal, D3D12 (handled by SDL3).
- **Auto-Logging:** Automatically logs the active **Graphics Backend** and **Shader Resource Counts** to the console on startup.
- **Clean C99:** Uses designated initializers for readable, zero-overhead pipeline creation.

## How it works

Instead of binding vertex buffers for every shape, this library:

1. Allocates one massive **Storage Buffer** on the GPU at startup.
2. Maps a pointer to CPU memory every frame via a Transfer Buffer.
3. Writes raw instance data (x, y, color, etc.) linearly to that pointer.
4. Uploads everything in one go at the end of the frame.
5. Pushes Screen Dimensions via **Uniforms** (Binding 0).
6. Uses a **Vertex Shader** to generate geometry on the fly using `SV_VertexID`.

## SDL3 SPIR-V Resource Mapping

SDL3 expects SPIR-V resources to be bound in specific sets and bindings. When writing your own GLSL shaders for `sgl`, use this table:

| Resource Type | GLSL Keyword | Set (Space) | Binding | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Uniform Buffers** | `uniform` | `set = 0` | `binding = 0` | Used for global data (Screen Size, Time). |
| **Storage Buffers** | `buffer` | `set = 0` | `binding = 1` | Used for Instance Data (Transforms, Colors). |
| **Textures** | `uniform sampler2D` | `set = 2` | `binding = 0` | (Future Use) Texture samplers. |

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
#version 450

// Binding 0: Uniforms (Screen Size)
// std140 ensures memory alignment matches C struct
layout(std140, set = 0, binding = 0) uniform ScreenUniforms {
    float screenW;
    float screenH;
    float padding1;
    float padding2;
};

// Binding 1: Instance Data (Storage Buffer)
// std430 is standard for SSBOs
struct InstanceData {
    vec4 rect;  // x, y, w, h
    vec4 color; // r, g, b, a
};

layout(std430, set = 0, binding = 1) readonly buffer Instances {
    InstanceData data[];
} instances;

// Output to Fragment Shader
layout(location = 0) out vec4 outColor;

void main() {
    // Pull Data for this specific instance
    // In Vulkan GLSL, use gl_InstanceIndex (not gl_InstanceID)
    InstanceData inst = instances.data[gl_InstanceIndex];

    // Generate Quad Geometry on the fly (Indices 0..5)
    vec2 corner;
    uint idx = gl_VertexIndex % 6; // Use gl_VertexIndex

    // Triangle 1
    if      (idx == 0) corner = vec2(0.0, 0.0);
    else if (idx == 1) corner = vec2(0.0, 1.0);
    else if (idx == 2) corner = vec2(1.0, 0.0);
    // Triangle 2
    else if (idx == 3) corner = vec2(1.0, 0.0);
    else if (idx == 4) corner = vec2(0.0, 1.0);
    else               corner = vec2(1.0, 1.0);

    // Calculate World Position (x + w * corner)
    vec2 worldPos = inst.rect.xy + (corner * inst.rect.zw);

    // Convert to Normalized Device Coordinates (NDC) using Uniforms
    vec2 ndc;
    ndc.x = (worldPos.x / screenW) * 2.0 - 1.0;
    ndc.y = (worldPos.y / screenH) * 2.0 - 1.0; // Y is down in Vulkan

    gl_Position = vec4(ndc, 0.0, 1.0);
    outColor = inst.color;
}
```

#### Fragment Shader (`shaders/default.frag`)

Save this as `shaders/default.frag`

```glsl
#version 450

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outFragColor;

void main() {
    outFragColor = inColor;
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
 sgl_InitWindow(800, 800, "Simple GPU Library for SDL3");

 SGL_Camera cam;
 sgl_CameraInit(&cam, 0.0f, 0.0f, 1.0f);

 // Raylib style: The loop condition handles input internally
 while (!sgl_WindowShouldClose()) {
        
        // Update Game Logic
  sgl_CameraUpdate(&cam);

        // Render
  sgl_Begin();
  sgl_SetCamera(&cam);

  sgl_DrawRectangle(10, 10, 100, 100, (SGL_COLOR){ 0, 255, 0, 255 }); 
  sgl_DrawTriangle(400, 300, 50, (SGL_COLOR){ 255, 0, 0, 255 });

  sgl_End();
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

- Implement `sgl_DrawTexture(SDL_Texture* tex, x, y, w, h)`

- Add `sgl_SetCamera(x, y, zoom)` to support scrolling and zooming. (but maybe will not happen).

- **Text Rendering:** Implement a Bitmap Font system where glyphs are treated as textured quads.

- **Z-Ordering:** Add a Depth Buffer (Z-Buffer) pipeline so sprites can be drawn in any order but sorted correctly on the GPU.
