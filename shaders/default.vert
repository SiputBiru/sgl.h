#version 450

// --- SET 0: Instance Data (Storage Buffer) ---
struct InstanceData {
    vec4 rect;   // x, y, w, h
    vec4 params; // x=angle, y=ox, z=oy, w=type
    vec4 color;  // r, g, b, a
};

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    InstanceData data[];
} instances;

// --- SET 1: Uniform Buffer ---
layout(set = 1, binding = 0) uniform ScreenUniforms {
    float screenW;
    float screenH;
    float camX, camY;
    float zoom;
    float padding; // Alignment padding
};

// --- Outputs to Fragment Shader ---
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) out float outType;

void main() {
    InstanceData inst = instances.data[gl_InstanceIndex];
    int type = int(inst.params.w);
    
    // Generate Base Geometry (0..1)
    vec2 corner;
    uint idx = gl_VertexIndex % 6;
    
    if (type == 1) { // Triangle
        if      (idx == 0) corner = vec2(0.5, 0.0); // Top Center
        else if (idx == 1) corner = vec2(0.0, 1.0); // Bottom Left
        else if (idx == 2) corner = vec2(1.0, 1.0); // Bottom Right
        else               corner = vec2(0.0, 0.0); // Degenerate
    } else { // Rect (0) or Circle (2)
        // Standard Quad vertices
        if      (idx == 0) corner = vec2(0, 0);
        else if (idx == 1) corner = vec2(0, 1);
        else if (idx == 2) corner = vec2(1, 0);
        else if (idx == 3) corner = vec2(1, 0);
        else if (idx == 4) corner = vec2(0, 1);
        else               corner = vec2(1, 1);
    }

    // Pass data to Fragment Shader
    outUV = corner; 
    outType = float(type);
    outColor = inst.color;

    // Apply Rotation (Model Space)
    // Scale local quad by width/height (rect.zw)
    vec2 localPos = corner * inst.rect.zw; 
    
    // Offset by origin (pivot point)
    localPos -= inst.params.yz; 
    
    // Rotate
    float theta = inst.params.x;
    float c = cos(theta); 
    float s = sin(theta);
    vec2 rotated = vec2(
        localPos.x * c - localPos.y * s, 
        localPos.x * s + localPos.y * c
    );
    
    // Translate to World Position
    vec2 worldPos = inst.rect.xy + rotated;

    // Apply Camera (World -> Screen Space)
    // Camera Position (camX, camY) represents the Top-Left of the screen
    vec2 cameraPos = vec2(camX, camY);
    
    // Apply Pan and Zoom
    vec2 viewPos = (worldPos - cameraPos) * zoom;

    // Convert to NDC (Normalized Device Coordinates)
    // Map 0..ScreenSize to -1..1
    gl_Position = vec4(
        (viewPos.x / screenW) * 2.0 - 1.0, 
        (viewPos.y / screenH) * 2.0 - 1.0, 
        0.0, 
        1.0
    );
}
