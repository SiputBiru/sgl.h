## Changelog

### [Unreleased]

- **Planned:** Texture Rendering support (Bindless).
- **Planned:** 3D Primitive Rendering (Cube, Sphere)

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
