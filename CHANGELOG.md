## Changelog

### [Unreleased]

- **Planned:** 3D Primitive Rendering (Sphere, Cylinder)
- **Planned:** Source Rectangle Support (Sprite Sheets)

### [2026-02-21] - Bindless Textures & VRAM Management

#### Added

- **Bindless Texture Arrays:** Replaced the legacy 8-slot binding system with a single global `sampler2DArray` (256 layers). The shader now dynamically pulls texture slices using a `texIndex` passed via the SSBO, enabling thousands of unique textures to be drawn in a single draw call without CPU binding overhead.

- **VRAM Free List Allocator:** Implemented a memory manager for the global texture array. `sgl_DestroyTexture` now recycles layer IDs into a stack, allowing `sgl_LoadTexture` to intelligently overwrite unused VRAM slices to prevent memory leaks.

- **Automatic Asset Standardization:** Integrated `SDL_ScaleSurface` into the loading pipeline. All loaded images are now automatically scaled (using Nearest Neighbor) to strictly fit the 512x512 array constraint, preventing Vulkan validation errors with arbitrary image sizes.

- **3D Camera Controller**: adding `sgl_Camera3DUpdate` to handle fly camera controller, and also adding new attribute of `type` in the SGL_Camera3D to change wheter want to use Free Fly or orbiting.

- **3D Cube texture things**: now sgl can load texture using `sgl_LoadTexture` then pass it to the `sgl_DrawCube`.

#### Changed

- **Shader Architecture:** Unified the Fragment Shader to use a single `globalTextures` binding, removing the limitation of only having 8 active textures per frame.
- **Texture API:** Updated `sgl_DrawTexture` to pass texture IDs purely as data in the Instance Buffer, completely decoupling rendering from resource binding.
- **New camera mode enum for 3d**: `SGL_CAMERA_MODE` will currently have `CAMERA_FREE` and `CAMERA_ORBITAL`, and also the `sgl_Camera3DUpdate` will need to specify the camera `mode` and also the `deltaTime`.
- **sgl_DrawCube**: will need to pass the texture but if not define/define by null, sgl will draw simple color things.

### [2026-02-18] - 3D Camera System & 3D Implementation

#### Added

- **Mode-Based Rendering API**: Introduced sgl_BeginMode3D() and sgl_BeginMode2D() to allow seamless switching between 3D perspective and 2D orthographic projections within a single frame.

- **3D Camera System**: Added the SGL_Camera3D structure, supporting position, target look-at points, up-vectors, and adjustable Field of View (FOV).

- **3D Primitive Support**: Added sgl_DrawCube(), which utilizes shader-side vertex pulling to generate a 36-vertex cube from a single instance data point.

- **Matrix Math Library**: Implemented internal math helpers for 4x4 matrix operations, including Identity, Orthographic, Perspective, and LookAt calculations.

- **Automatic Batch Flushing**: Updated the internal renderer to automatically detect when a mode switch (2D to 3D) occurs, triggering a GPU flush to handle the different vertex counts (6 for quads vs. 36 for cubes).

- **Vulkan-Compatible Depth Mapping**: Optimized the perspective projection matrix to map depth to the $[0, 1]$ range, ensuring compatibility with the SDL3 GPU/Vulkan standard.

### [2026-02-16] - Logger & 2D Texture

#### Added

- **SGL logger:**
  - Added `SGL_Log`, `SGL_Warn`, and `SGL_Error`.
- **2D Texture loader:**
  - Added `sgl_DrawTexture`
  - the fragment shaders now have unrolled bindings (later will be refactoring this to properly load other image format and other things idk Man)

  ```glsl
  layout(set = 2, binding = 0) uniform sampler2D tex0;
  layout(set = 2, binding = 1) uniform sampler2D tex1;
  layout(set = 2, binding = 2) uniform sampler2D tex2;
  layout(set = 2, binding = 3) uniform sampler2D tex3;
  layout(set = 2, binding = 4) uniform sampler2D tex4;
  layout(set = 2, binding = 5) uniform sampler2D tex5;
  layout(set = 2, binding = 6) uniform sampler2D tex6;
  layout(set = 2, binding = 7) uniform sampler2D tex7;
  ```

### [2026-02-15] - Camera System & Raylib-Style Input

#### Added

- **Camera System:** Introduced a 2D Camera API with zoom and pan support.
  - Added `sgl_CameraInit`, `sgl_CameraUpdate` (WASD + Zoom), and `sgl_SetCamera`.
  - Updated Vertex Shader to apply View/Projection matrices via Uniforms.
- **Input Management:** Added Raylib-style input polling helpers.
  - `sgl_WindowShouldClose()`: Manages the `SDL_Event` loop and closing logic internally.
  - `sgl_IsKeyDown()`: Checks real-time keyboard state for smooth input handling.
- **Depth Stencil:** Implemented proper Depth Texture creation (`D16_UNORM`) to handle depth testing for the new camera and future 3D features.
- **New Shapes:** Added dedicated functions for drawing advanced primitives.

```C
void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_COLOR color);
void sgl_DrawRectanglePro(
    Rectangle rec, Vec2 origin, float rotation,
    SGL_COLOR color);

void sgl_DrawTriangle(f32 x, f32 y, f32 size, SGL_COLOR color);

void sgl_DrawCircle(f32 x, f32 y, f32 radius, SGL_COLOR color);

```

#### Changed

- **Shader Protocol**: Updated `default.vert` and `default.frag` to support camera uniforms (`camX`, `camY`, `zoom`) and pass Shape Type as `float` (Location 2) to fix Vulkan validation errors.

- **Resource Management**: Updated `sgl_Shutdown` to properly release the internal depthTexture to prevent Vulkan validation errors on exit (`VUID-vkDestroyDevice-device-05137`).

### [2026-02-14] - Embedded Shaders & Refactor

#### Added

- **Embedded Shaders:** Default shaders are now compiled into byte arrays (`sgl_default_vert_spv` / `sgl_default_frag_spv`) directly inside `sgl.h`.
  - *Benefit:* no longer need to ship or load a separate `shaders/` folder.
- **Logging:** `sgl_InitWindow` now logs the active **Graphics Backend** (Vulkan, Metal, or D3D12) to the console for easier debugging.
- **Debug Info:** `sgl_LoadShader` now logs the number of Uniform and Storage buffers bound to each shader.

#### Changed

- **Shaders:** Ported default shaders from HLSL to **GLSL** (`.vert` / `.frag`) to improve compatibility with Vulkan drivers.
- **API:** Made `sgl_InternalInit` private (static); users now exclusively use `sgl_InitWindow` to initialize the library.

#### Fixed

- Fixed an issue where the pipeline configuration wasn't properly clearing zero-initialized structs, potentially causing undefined behavior on some backends.

### [2026-02-10] - Initial Release

#### Added

- **Core:** Implemented "Vertex Pulling" rendering architecture using Storage Buffers (SSBO).
- **Drawing:** Added `sgl_DrawRectangle(x, y, w, h, color)` for immediate-mode shape rendering.
- **System:** Basic Window creation and GPU Pipeline initialization using SDL3.
