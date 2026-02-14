## Changelog

### [Unreleased]

- **Planned:** Texture Rendering support (Bindless).
- **Planned:** Camera system (View/Projection Matrices).

### [2026-02-14] - Embedded Shaders & Refactor

#### Added

- **Embedded Shaders:** Default shaders are now compiled into byte arrays (`sgl_default_vert_spv` / `sgl_default_frag_spv`) directly inside `sgl.h`.
  - *Benefit:* You no longer need to ship or load a separate `shaders/` folder.
- **Logging:** `sgl_InitWindow` now automatically logs the active **Graphics Backend** (Vulkan, Metal, or D3D12) to the console for easier debugging.
- **Debug Info:** `sgl_LoadShader` now logs the number of Uniform and Storage buffers bound to each shader.

#### Changed

- **Refactor:** Replaced all `SDL_zero()` macros with cleaner C99 designated initializers for better readability and safety.
- **Shaders:** Ported default shaders from HLSL to **GLSL** (`.vert` / `.frag`) to improve compatibility with Vulkan drivers.
- **API:** Made `sgl_InternalInit` private (static); users now exclusively use `sgl_InitWindow` to initialize the library.

#### Fixed

- Fixed an issue where the pipeline configuration wasn't properly clearing zero-initialized structs, potentially causing undefined behavior on some backends.

### [2026-02-10] - Initial Release

#### Added

- **Core:** Implemented "Vertex Pulling" rendering architecture using Storage Buffers (SSBO).
- **Drawing:** Added `sgl_DrawRectangle(x, y, w, h, color)` for immediate-mode shape rendering.
- **System:** Basic Window creation and GPU Pipeline initialization using SDL3.
