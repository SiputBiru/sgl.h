/*
    sgl.h - Simple GPU Library for SDL3
    Implements "Vertex Pulling" and "Big Buffer" rendering.
*/

#ifndef SGL_H
#define SGL_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef float f32;

// Config
#define SGL_MAX_INSTANCES 10000
#define SGL_FRAMES_IN_FLIGHT 3

typedef struct {
    unsigned char r, g, b, a;
} SGL_COLOR;

// API
bool sgl_Init(SDL_Window *window, SDL_GPUDevice *device);
void sgl_Shutdown(void);

// Frame Control
void sgl_Begin(SDL_GPUCommandBuffer *cmd);
void sgl_End(SDL_Window *window);

// Drawing
void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_COLOR color);

#endif // SGL_H

#ifdef SGL_IMPLEMENTATION

// Data Layout
typedef struct {
    f32 x, y, w, h;
    f32 r, g, b, a;
} SGL_InstanceData;

// Internal State
static struct {
    SDL_GPUDevice *device;
    SDL_Window *window;
    SDL_GPUGraphicsPipeline *pipeline;

    SDL_GPUBuffer *instanceBuffer;
    SDL_GPUTransferBuffer *transferBuffer;

    SGL_InstanceData *mappedPtr;

    Uint32 instanceCount;

    Uint32 frameIndex;
    Uint32 frameSize;
    Uint32 frameOffset;

    SDL_GPUCommandBuffer *curCmd;
    int winW, winH;
} sgl;

// Internal Helper: Set Viewport
static void sgl_SetViewport(SDL_GPURenderPass *pass) {
    SDL_GPUViewport viewport = {.x = 0.0f,
                                .y = 0.0f,
                                .w = (f32)sgl.winW,
                                .h = (f32)sgl.winH,
                                .min_depth = 0.0f,
                                .max_depth = 1.0f};
    SDL_SetGPUViewport(pass, &viewport);

    SDL_Rect scissor = {0, 0, sgl.winW, sgl.winH};
    SDL_SetGPUScissor(pass, &scissor);
}

// Internal Helper: Load Shader
// static SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
//                                  int stage, int num_storage, int num_uniform)
//                                  {
//     size_t codeSize;
//     void *code = SDL_LoadFile(filename, &codeSize);
//     if (!code) {
//         SDL_Log("Failed to load shader: %s", filename);
//         return NULL;
//     }
//
//     SDL_Log("Loading Shader %s: Storage=%d, Uniform=%d", filename,
//     num_storage,
//             num_uniform);
//
//     SDL_GPUShaderCreateInfo shaderInfo = {
//         .code = (const uint8_t *)code,
//         .code_size = codeSize,
//         .entrypoint = "main",
//         .format = SDL_GPU_SHADERFORMAT_SPIRV,
//         .stage = (SDL_GPUShaderStage)stage,
//         .num_storage_buffers = (Uint32)num_storage,
//         .num_uniform_buffers = (Uint32)num_uniform};
//
//     SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shaderInfo);
//     SDL_free(code);
//     return shader;
// }

// In sgl.h

static SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
                                 int stage, int num_storage, int num_uniform) {
    size_t codeSize;
    void *code = SDL_LoadFile(filename, &codeSize);
    if (!code) {
        SDL_Log("Failed to load shader: %s", filename);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderInfo;
    SDL_memset(&shaderInfo, 0, sizeof(shaderInfo));

    shaderInfo.code_size = codeSize;
    shaderInfo.code = (const uint8_t *)code;
    shaderInfo.entrypoint = "main";
    shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    shaderInfo.stage = (SDL_GPUShaderStage)stage;

    // Explicitly set these to 0
    shaderInfo.num_samplers = 0;
    shaderInfo.num_storage_textures = 0;

    // Assign your requested counts
    shaderInfo.num_storage_buffers = (Uint32)num_storage;
    shaderInfo.num_uniform_buffers = (Uint32)num_uniform;

    // shaderInfo.props = 0;

    SDL_Log("Loading Shader %s: Stage=%d Storage=%d, Uniform=%d", filename,
            shaderInfo.stage, shaderInfo.num_storage_buffers,
            shaderInfo.num_uniform_buffers);

    // SDL_Log("DEBUG: Forcing Storage=1, Uniform=1");
    // shaderInfo.num_storage_buffers = 1;
    // shaderInfo.num_uniform_buffers = 1;

    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shaderInfo);
    SDL_free(code);
    return shader;
}

bool sgl_Init(SDL_Window *window, SDL_GPUDevice *device) {
    sgl.device = device;

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("sgl_Init: Could not claim window for GPU Device");
        return false;
    }

    sgl.window = window;
    SDL_GetWindowSize(window, &sgl.winW, &sgl.winH);

    sgl.frameSize = SGL_MAX_INSTANCES * sizeof(SGL_InstanceData);

    SDL_GPUBufferCreateInfo bufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = sgl.frameSize * SGL_FRAMES_IN_FLIGHT};

    sgl.instanceBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    SDL_GPUTransferBufferCreateInfo transferinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = SGL_MAX_INSTANCES * sizeof(SGL_InstanceData)};

    sgl.transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferinfo);

    // Vertex: 1 Storage (Space 1), 1 Uniform (Space 2)
    SDL_GPUShader *vertShader = LoadShader(device, "shaders/shader.vert.spv",
                                           SDL_GPU_SHADERSTAGE_VERTEX, 1, 1);

    // Fragment: 0 Storage, 0 Uniform
    SDL_GPUShader *fragShader = LoadShader(device, "shaders/shader.frag.spv",
                                           SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    if (!vertShader || !fragShader)
        return false;

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

    return (sgl.pipeline != NULL);
}

void sgl_Begin(SDL_GPUCommandBuffer *cmd) {
    sgl.curCmd = cmd;
    sgl.instanceCount = 0;

    SDL_GetWindowSize(sgl.window, &sgl.winW, &sgl.winH);

    sgl.frameIndex = (sgl.frameIndex + 1) % SGL_FRAMES_IN_FLIGHT;

    sgl.frameOffset = sgl.frameIndex * sgl.frameSize;

    sgl.mappedPtr = (SGL_InstanceData *)SDL_MapGPUTransferBuffer(
        sgl.device, sgl.transferBuffer, true);
}

void sgl_DrawRectangle(f32 x, f32 y, f32 w, f32 h, SGL_COLOR color) {
    if (sgl.instanceCount >= SGL_MAX_INSTANCES)
        return;
    if (sgl.mappedPtr == NULL)
        return;

    SGL_InstanceData *data = &sgl.mappedPtr[sgl.instanceCount++];
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

    Uint32 usedSize = sgl.instanceCount * sizeof(SGL_InstanceData);

    if (sgl.instanceCount > 0) {
        SDL_GPUTransferBufferLocation src = {
            .transfer_buffer = sgl.transferBuffer, .offset = 0};

        SDL_GPUBufferRegion dst = {.buffer = sgl.instanceBuffer,
                                   .offset = sgl.frameOffset,
                                   .size = usedSize};

        SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(sgl.curCmd);

        SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);

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
                sgl_SetViewport(pass);

                Uint32 baseOffset = sgl.frameOffset / sizeof(SGL_InstanceData);

                struct {
                    f32 w, h;
                    Uint32 baseOffset;
                    Uint32 padding;
                } uniforms = {(f32)sgl.winW, (f32)sgl.winH, baseOffset, 0};
                SDL_PushGPUVertexUniformData(sgl.curCmd, 0, &uniforms,
                                             sizeof(uniforms));

                // SDL_BindGPUVertexStorageBuffers(pass, 0, &sgl.instanceBuffer,
                //                                 1);

                // SDL_GPUBufferRegion region = {.buffer = sgl.instanceBuffer,
                //                               .offset = sgl.frameOffset,
                //                               .size = usedSize};
                //
                SDL_GPUBuffer *bufs[] = {sgl.instanceBuffer};

                SDL_BindGPUVertexStorageBuffers(pass, 0, bufs, 1);

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
