#version 450

// --- SET 0: Instance Data ---
struct InstanceData {
    vec4 rect;    // [x, y, z, size]
    vec4 params;  // [angle, ox, oy, unused]
    vec4 params2; // [type, texIndex, p2, p3]
    vec4 color;   // [r, g, b, a]
};

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    InstanceData data[];
} instances;

// --- SET 1: Uniforms ---
layout(set = 1, binding = 0) uniform Uniforms {
    mat4 mvp;
};

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) flat out int outTexIndex;
layout(location = 3) flat out float outType;

// Cube Vertices
const vec3 cubeVerts[36] = vec3[36](
    vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5, -0.5,  0.5),
    vec3( 0.5, -0.5, -0.5), vec3(-0.5, -0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5, -0.5, -0.5),
    vec3(-0.5,  0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3(-0.5, -0.5,  0.5), vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5,  0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5, -0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3(-0.5, -0.5,  0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5, -0.5, -0.5) 
);

void main() {
    InstanceData inst = instances.data[gl_InstanceIndex];
    int type = int(inst.params2.x);
    vec3 localPos;

    if (type == 100) { // CUBE
        localPos = cubeVerts[gl_VertexIndex % 36];
        localPos *= inst.rect.w; 
        localPos += inst.rect.xyz;
        outUV = vec2(0.0);
        outColor = inst.color; // Simplify lighting for now
    } else { // 2D (Rect/Tri/Circle)
        vec2 corner;
        uint idx = gl_VertexIndex;
        
        // DEGENERATE TRIANGLES: Throw away extra vertices for 2D
        if (idx >= 6) corner = vec2(0.0);
        else if (type == 1) { // Triangle
             if (idx == 0) corner = vec2(0.5, 0.0); else if (idx == 1) corner = vec2(0.0, 1.0); else corner = vec2(1.0, 1.0);
        } else { // Quad
             if (idx == 0) corner = vec2(0, 0); else if (idx == 1) corner = vec2(0, 1); else if (idx == 2) corner = vec2(1, 0);
             else if (idx == 3) corner = vec2(1, 0); else if (idx == 4) corner = vec2(0, 1); else corner = vec2(1, 1);
        }
        
        outUV = vec2(corner.x, 1.0 - corner.y); // Flip Y for SDL/Vulkan
        
        // 2D Rotation
        vec2 p = (corner * inst.rect.zw) - inst.params.yz;
        float c = cos(inst.params.x), s = sin(inst.params.x);
        
        localPos = vec3(inst.rect.xy + vec2(p.x*c - p.y*s, p.x*s + p.y*c), inst.rect.z);
        outColor = inst.color;
    }

    outType = float(type);
    gl_Position = mvp * vec4(localPos, 1.0);
    outTexIndex = int(inst.params2.y);
}
