#version 450

struct SGL_InstanceData {
    float x, y, w, h;
    float r, g, b, a;
};

// SDL3 Convention: Vertex Storage Buffers go in SET 0
layout(std430, set = 0, binding = 0) readonly buffer InstanceBuffer {
    SGL_InstanceData instances[];
};

// SDL3 Convention: Vertex Uniform Buffers go in SET 1
layout(set = 1, binding = 0) uniform WindowUniforms {
    float winW, winH;
    float pad1, pad2;
};

layout(location = 0) out vec4 fragColor;

void main() {
    SGL_InstanceData data = instances[gl_InstanceIndex];

    vec2 pos;
    int idx = gl_VertexIndex % 6;
    
    if (idx == 0)      pos = vec2(data.x, data.y);
    else if (idx == 1) pos = vec2(data.x, data.y + data.h);
    else if (idx == 2) pos = vec2(data.x + data.w, data.y);
    else if (idx == 3) pos = vec2(data.x + data.w, data.y);
    else if (idx == 4) pos = vec2(data.x, data.y + data.h);
    else               pos = vec2(data.x + data.w, data.y + data.h);

    // Convert pixels to NDC (-1 to 1)
    float ndcX = (2.0 * pos.x / winW) - 1.0;
    float ndcY = (2.0 * pos.y / winH) - 1.0;

    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
    fragColor = vec4(data.r, data.g, data.b, data.a);
}
