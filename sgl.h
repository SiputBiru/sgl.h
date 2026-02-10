/*
    sgl.h - Simple GPU Library for SDL3
    Implements "Vertex Pulling" and "Big Buffer" rendering.

    Define SGL_IMPLEMENTATION in ONE .c file before including this header.
*/

#ifndef SGL_H

#define SGL_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <stdbool.h>
#include <stdint.h>
// #include <SDL3/SDL_gpu.h>

// let's assume float is 32 bit floating point number
typedef float f32;

// Config
#define SGL_MAX_INSTANCES 10000
#define SGL_WIN_WIDTH 800
#define SGL_WIN_HEIGHT 600

typedef struct {
    f32 x, y, w, h;
} SGL_RECT;

typedef struct {
    unsigned char r, g, b, a;
} SGL_COLOR;

// API
bool sgl_Init(SDL_Window *window, SDL_GPUDevice *device);
void sgl_Shutdown(void);

// Frame Control
void sgl_Begin(SDL_GPUCommandBuffer *cmd);
void sgl_End(SDL_Window *window);

// Shader helper function
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
                          int stage, int num_storage_buffers);

// Drawing (almost like Raylib style)
void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_COLOR color);

#endif // SGL_H

#ifdef SGL_IMPLEMENTATION

// Data Layout for GPU (Matches Shader Structure)
typedef struct {
    f32 x, y, w, h; // Position & Size
    f32 r, g, b, a; // Color (normalized 0-1)
    f32 padding[8]; // Pad to 16-byte alignment (std140/std430 rules)
} SGL_InstanceData;

// Internal State
static struct {
    SDL_GPUDevice *device;
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *instanceBuffer;
    SDL_GPUTransferBuffer *transferBuffer;
    SGL_InstanceData *mappedPtr;
    Uint32 instanceCount;
    SDL_GPUCommandBuffer *curCmd;
    int winW, winH;
} sgl;

// Shader Compilation helpter is omitted for brevity
// compiled shader are needed to make code below works

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
                          int stage, int num_storage_buffers) {
    size_t codeSize;
    void *code = SDL_LoadFile(filename, &codeSize);
    if (!code) {
        SDL_Log("Failed to load shader: %s", filename);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code = (const uint8_t *)code, //  Explicit cast to const Uint8*
        .code_size = codeSize,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = (SDL_GPUShaderStage)stage,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = (uint32_t)num_storage_buffers, //  Cast to Uint32
        .num_uniform_buffers = 0};

    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shaderInfo);
    SDL_free(code);
    return shader;
}

bool sgl_Init(SDL_Window *window, SDL_GPUDevice *device) {
    sgl.device = device;

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("sgl_Init: Cloud not claim window for GPU Device");
        return false;
    }

    SDL_GetWindowSize(window, &sgl.winW, &sgl.winH);

    // Create One Big Buffer
    // assume data won't exceed MAX_INSTANCES per frame
    SDL_GPUBufferCreateInfo bufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = SGL_MAX_INSTANCES * sizeof(SGL_InstanceData)};

    sgl.instanceBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    // Create Transfer Buffer for CPU -> GPU upload
    SDL_GPUTransferBufferCreateInfo transferinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = SGL_MAX_INSTANCES * sizeof(SGL_InstanceData)};
    sgl.transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferinfo);

    // Create Pipeline (Vertex Pulling - No Vertex Input State needed!)
    // Load Shader here....
    SDL_GPUShader *vertShader = LoadShader(device, "shaders/shader.vert.spv",
                                           SDL_GPU_SHADERSTAGE_VERTEX, 1);
    SDL_GPUShader *fragShader = LoadShader(device, "shaders/shader.frag.spv",
                                           SDL_GPU_SHADERSTAGE_FRAGMENT, 0);

    if (!vertShader || !fragShader) {
        SDL_Log("Failed to load shaders! check your file paths.");
        return false;
    }

    SDL_GPUColorTargetDescription targetDesc = {
        .format = SDL_GetGPUSwapchainTextureFormat(device, window),
        .blend_state = {
            .enable_blend = true,
            .color_write_mask = 0xF,
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        }};

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
        .vertex_shader = vertShader,
        .fragment_shader = fragShader,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = {.num_color_targets = 1,
                        .color_target_descriptions = &targetDesc},

    };

    sgl.pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

    SDL_ReleaseGPUShader(device, vertShader);
    SDL_ReleaseGPUShader(device, fragShader);

    if (sgl.pipeline == NULL) {
        SDL_Log("Failed to create pipeline !Check shader format vs Backend.");
        return false;
    }

    return true;
}

void sgl_Begin(SDL_GPUCommandBuffer *cmd) {
    sgl.curCmd = cmd;
    sgl.instanceCount = 0;

    sgl.mappedPtr = (SGL_InstanceData *)SDL_MapGPUTransferBuffer(
        sgl.device, sgl.transferBuffer,
        true // Cycle (overwrite previous frame's data safely)
    );
}

void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_COLOR color) {
    if (sgl.instanceCount >= SGL_MAX_INSTANCES)
        return;

    // Linear Allocation
    if (sgl.mappedPtr == NULL) {
        return;
    }

    SGL_InstanceData *data = &sgl.mappedPtr[sgl.instanceCount++];

    // Coordinate Conversion (Pixel to NDC -1..1 logic can happen here or in the
    // shader) pass raw pixel coords to keep C side simple
    data->x = x;
    data->y = y;
    data->w = w;
    data->h = h;
    data->r = color.r / 255.0f;
    data->g = color.g / 255.0f;
    data->b = color.b / 255.0f;
    data->a = color.a / 255.0f;
}

void sgl_End(SDL_Window *window) {
    SDL_UnmapGPUTransferBuffer(sgl.device, sgl.transferBuffer);

    if (sgl.instanceCount > 0) {
        // Upload CPU data to big GPU Buffer
        SDL_GPUTransferBufferLocation src = {
            .transfer_buffer =
                sgl.transferBuffer, // pass the buffer from the sgl
            .offset = 0,
        };

        SDL_GPUBufferRegion dst = {
            .buffer = sgl.instanceBuffer,
            .offset = 0, // offset
            .size = (Uint32)(sgl.instanceCount * sizeof(SGL_InstanceData))};

        SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(sgl.curCmd);
        SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
        SDL_EndGPUCopyPass(copyPass);

        SDL_GPUTexture *swapchainTexture;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                sgl.curCmd, window, &swapchainTexture, NULL, NULL))
            return;

        SDL_GPUColorTargetInfo colorTarget = {
            .texture = swapchainTexture,
            .clear_color = {0.1f, 0.1f, 0.1f, 1.0f},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE};

        SDL_GPURenderPass *pass =
            SDL_BeginGPURenderPass(sgl.curCmd, &colorTarget, 1, NULL);

        if (pass) {
            if (sgl.pipeline) {
                SDL_BindGPUGraphicsPipeline(pass, sgl.pipeline);
                // Bind the Instance buffer to slot 0 (read-only storage)
                SDL_BindGPUVertexStorageBuffers(pass, 0, &sgl.instanceBuffer,
                                                1);

                SDL_DrawGPUPrimitives(pass, 6, sgl.instanceCount, 0, 0);
            }

            SDL_EndGPURenderPass(pass);
        }
    }
}

void sgl_Shutdown(void) {
    SDL_ReleaseGPUBuffer(sgl.device, sgl.instanceBuffer);

    SDL_ReleaseGPUTransferBuffer(sgl.device, sgl.transferBuffer);

    SDL_ReleaseGPUGraphicsPipeline(sgl.device, sgl.pipeline);
}

#endif
