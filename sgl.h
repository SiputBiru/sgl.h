/*
    sgl.h - Simple GPU Library for SDL3
    Implements "Vertex Pulling" and "Big Buffer" rendering.
*/

#ifndef SGL_H
#define SGL_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <stdbool.h>
#include <stdint.h>

typedef float f32;
#define SGL_MAX_INSTANCES 10000

typedef struct {
    uint8_t r, g, b, a;
} SGL_Color;

// -- API --
void sgl_InitWindow(int w, int h, const char *title);
void sgl_Shutdown(void);

// Custom Shader API
SDL_GPUShader *sgl_LoadShader(const char *filename, SDL_GPUShaderStage stage,
                              int num_uniform, int num_storage);
SDL_GPUGraphicsPipeline *sgl_CreatePipeline(SDL_GPUShader *vert,
                                            SDL_GPUShader *frag);

// Pipeline Control
void sgl_SetPipeline(SDL_GPUGraphicsPipeline *pipeline);
SDL_GPUGraphicsPipeline *sgl_GetDefaultPipeline(void);

// Drawing
void sgl_Begin(void);
void sgl_End(void);
void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_Color color);

#endif // SGL_H

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#ifdef SGL_IMPLEMENTATION

typedef struct {
    f32 x, y, w, h;
    f32 r, g, b, a;
} SGL_InstanceData;

static struct {
    SDL_GPUDevice *device;
    SDL_Window *window;
    SDL_GPUGraphicsPipeline *defaultPipeline;
    SDL_GPUGraphicsPipeline *activePipeline;
    SDL_GPUBuffer *instanceBuffer;
    SDL_GPUTransferBuffer *transferBuffer;
    SGL_InstanceData *mappedPtr;
    Uint32 instanceCount;
    SDL_GPUCommandBuffer *curCmd;
    int winW, winH;
} sgl;

// --- Helper Functions ---

static void sgl_SetViewport(SDL_GPURenderPass *pass) {
    SDL_GPUViewport viewport = {0, 0, (f32)sgl.winW, (f32)sgl.winH, 0.0f, 1.0f};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_Rect scissor = {0, 0, sgl.winW, sgl.winH};
    SDL_SetGPUScissor(pass, &scissor);
}

// --- Shader API ---

SDL_GPUShader *sgl_LoadShader(const char *filename, SDL_GPUShaderStage stage,
                              int num_uniform, int num_storage) {
    size_t codeSize;
    void *code = SDL_LoadFile(filename, &codeSize);
    if (!code) {
        SDL_Log("SGL Error: Shader file not found: %s", filename);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = codeSize,
        .code = (const uint8_t *)code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = (Uint32)num_storage,
        .num_uniform_buffers = (Uint32)num_uniform};

    SDL_GPUShader *shader = SDL_CreateGPUShader(sgl.device, &shaderInfo);
    SDL_free(code);

    if (shader) {
        SDL_Log(
            "SGL: Loaded Shader '%s' [Stage: %d | Uniforms: %d | Storage: %d]",
            filename, stage, num_uniform, num_storage);
    } else {
        SDL_Log("SGL Error: Failed to create shader from %s", filename);
    }

    return shader;
}

SDL_GPUGraphicsPipeline *sgl_CreatePipeline(SDL_GPUShader *vert,
                                            SDL_GPUShader *frag) {

    // Clean C99 Initialization (No SDL_zero needed)
    SDL_GPUColorTargetDescription targetDesc = {
        .format = SDL_GetGPUSwapchainTextureFormat(sgl.device, sgl.window),
        .blend_state = {
            .enable_blend = true,
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
            .color_write_mask = 0xF}};

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = {.num_color_targets = 1,
                        .color_target_descriptions = &targetDesc}};

    return SDL_CreateGPUGraphicsPipeline(sgl.device, &pipelineInfo);
}

// --- Init/Shutdown ---

bool sgl_InternalInit(SDL_Window *window, SDL_GPUDevice *device) {
    sgl.device = device;
    sgl.window = window;

    // Check and Log Backend
    const char *backend = SDL_GetGPUDeviceDriver(sgl.device);
    SDL_Log("SGL: Initialized. Graphics Backend: %s", backend);

    SDL_ClaimWindowForGPUDevice(device, window);
    SDL_GetWindowSize(window, &sgl.winW, &sgl.winH);

    SDL_GPUBufferCreateInfo bInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = SGL_MAX_INSTANCES * sizeof(SGL_InstanceData)};
    sgl.instanceBuffer = SDL_CreateGPUBuffer(device, &bInfo);

    SDL_GPUTransferBufferCreateInfo tInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = SGL_MAX_INSTANCES * sizeof(SGL_InstanceData)};
    sgl.transferBuffer = SDL_CreateGPUTransferBuffer(device, &tInfo);

    // LOAD DEFAULT SHADERS
    SDL_GPUShader *v = sgl_LoadShader("shaders/default.vert.spv",
                                      SDL_GPU_SHADERSTAGE_VERTEX, 1, 1);
    SDL_GPUShader *f = sgl_LoadShader("shaders/default.frag.spv",
                                      SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    if (v && f) {
        sgl.defaultPipeline = sgl_CreatePipeline(v, f);
        sgl.activePipeline = sgl.defaultPipeline;
        SDL_ReleaseGPUShader(device, v);
        SDL_ReleaseGPUShader(device, f);
    } else {
        SDL_Log("SGL Warning: Default shaders (default.vert.spv/frag.spv) not "
                "found!");
    }

    return true;
}

void sgl_InitWindow(int w, int h, const char *title) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);

    // Initialize GPU with standard SPIR-V format support
    SDL_GPUDevice *dev =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

    sgl_InternalInit(win, dev);
}

void sgl_Shutdown() {
    if (sgl.instanceBuffer)
        SDL_ReleaseGPUBuffer(sgl.device, sgl.instanceBuffer);
    if (sgl.transferBuffer)
        SDL_ReleaseGPUTransferBuffer(sgl.device, sgl.transferBuffer);
    if (sgl.defaultPipeline)
        SDL_ReleaseGPUGraphicsPipeline(sgl.device, sgl.defaultPipeline);

    SDL_DestroyGPUDevice(sgl.device);
    SDL_DestroyWindow(sgl.window);
    SDL_Quit();
}

// --- Pipeline Switching ---

void sgl_SetPipeline(SDL_GPUGraphicsPipeline *pipeline) {
    sgl.activePipeline = pipeline ? pipeline : sgl.defaultPipeline;
}

SDL_GPUGraphicsPipeline *sgl_GetDefaultPipeline(void) {
    return sgl.defaultPipeline;
}

// --- Drawing ---

void sgl_Begin() {
    sgl.curCmd = SDL_AcquireGPUCommandBuffer(sgl.device);
    sgl.instanceCount = 0;
    SDL_GetWindowSize(sgl.window, &sgl.winW, &sgl.winH);
    sgl.mappedPtr = (SGL_InstanceData *)SDL_MapGPUTransferBuffer(
        sgl.device, sgl.transferBuffer, true);
}

void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_Color color) {
    if (sgl.instanceCount >= SGL_MAX_INSTANCES)
        return;

    sgl.mappedPtr[sgl.instanceCount++] = (SGL_InstanceData){x,
                                                            y,
                                                            w,
                                                            h,
                                                            color.r / 255.0f,
                                                            color.g / 255.0f,
                                                            color.b / 255.0f,
                                                            color.a / 255.0f};
}

void sgl_End() {
    SDL_UnmapGPUTransferBuffer(sgl.device, sgl.transferBuffer);

    SDL_GPUTransferBufferLocation src = {.transfer_buffer = sgl.transferBuffer,
                                         .offset = 0};
    SDL_GPUBufferRegion dst = {
        .buffer = sgl.instanceBuffer,
        .offset = 0,
        .size = (Uint32)(sgl.instanceCount * sizeof(SGL_InstanceData))};

    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(sgl.curCmd);
    SDL_UploadToGPUBuffer(copy, &src, &dst, true);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUTexture *swapTexture;
    SDL_WaitAndAcquireGPUSwapchainTexture(sgl.curCmd, sgl.window, &swapTexture,
                                          NULL, NULL);

    if (swapTexture) {
        SDL_GPUColorTargetInfo colorTarget = {.texture = swapTexture,
                                              .clear_color = {0, 0, 0, 1},
                                              .load_op = SDL_GPU_LOADOP_CLEAR,
                                              .store_op =
                                                  SDL_GPU_STOREOP_STORE};

        SDL_GPURenderPass *pass =
            SDL_BeginGPURenderPass(sgl.curCmd, &colorTarget, 1, NULL);

        SDL_BindGPUGraphicsPipeline(pass, sgl.activePipeline);
        sgl_SetViewport(pass);

        // Upload Uniforms (Screen Size)
        struct {
            f32 w, h, p1, p2;
        } uni = {(f32)sgl.winW, (f32)sgl.winH, 0, 0};
        SDL_PushGPUVertexUniformData(sgl.curCmd, 0, &uni, sizeof(uni));

        // Bind Storage Buffer (Instance Data)
        SDL_GPUBuffer *bufs[] = {sgl.instanceBuffer};
        SDL_BindGPUVertexStorageBuffers(pass, 0, bufs, 1);

        SDL_DrawGPUPrimitives(pass, 6, sgl.instanceCount, 0, 0);
        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(sgl.curCmd);
}

#endif // SGL_IMPLEMENTATIONendif
