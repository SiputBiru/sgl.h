#version 450

// --- BINDING 0 (SET 0): Storage Buffer (Instance Data) ---
struct InstanceData {
    vec4 rect;   // x, y, w, h
    vec4 params; // x=angle, y=ox, z=oy, w=type
    vec4 color;  // r, g, b, a
};

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    InstanceData data[];
} instances;

// --- BINDING 0 (SET 1): Uniform Buffer ---
layout(set = 1, binding = 0) uniform ScreenUniforms {
    float screenW;
    float screenH;
    float padding1;
    float padding2;
};

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) out flat int outType;

void main() {
    InstanceData inst = instances.data[gl_InstanceIndex];
    int type = int(inst.params.w);
    
    // Generate Base Geometry (0..1)
    vec2 corner;
    uint idx = gl_VertexIndex % 6;
    
    if (type == 1) { // Triangle
        if      (idx == 0) corner = vec2(0.5, 0.0);
        else if (idx == 1) corner = vec2(0.0, 1.0);
        else if (idx == 2) corner = vec2(1.0, 1.0);
        else               corner = vec2(0.0, 0.0);
    } else { // Rect/Circle
        if      (idx == 0) corner = vec2(0, 0);
        else if (idx == 1) corner = vec2(0, 1);
        else if (idx == 2) corner = vec2(1, 0);
        else if (idx == 3) corner = vec2(1, 0);
        else if (idx == 4) corner = vec2(0, 1);
        else               corner = vec2(1, 1);
    }

    outUV = corner; 
    outType = type;
    outColor = inst.color;

    // Rotation & Position
    vec2 localPos = corner * inst.rect.zw; 
    localPos -= inst.params.yz; 
    
    float theta = inst.params.x;
    float c = cos(theta); 
    float s = sin(theta);
    vec2 rotated = vec2(localPos.x * c - localPos.y * s, localPos.x * s + localPos.y * c);
    
    vec2 worldPos = inst.rect.xy + rotated;

    // NDC Conversion
    gl_Position = vec4(
        (worldPos.x / screenW) * 2.0 - 1.0, 
        (worldPos.y / screenH) * 2.0 - 1.0, 
        0.0, 1.0
    );
}
