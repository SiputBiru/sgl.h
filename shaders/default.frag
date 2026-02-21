#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) flat in int inTexIndex;
layout(location = 3) flat in float inType;

// Binding 0: One massive texture array
layout(set = 2, binding = 0) uniform sampler2DArray globalTextures;

layout(location = 0) out vec4 outFragColor;

void main() {
    int type = int(inType);

    // Circle Logic (Discard pixels outside radius)
    if (type == 2) { 
        if (length(inUV - 0.5) > 0.5) discard;
    }

    // Texture Logic
    if (inTexIndex >= 0) {
        // Sample from the array using the index as the "Z" coordinate
        vec4 texColor = texture(globalTextures, vec3(inUV.x, inUV.y, float(inTexIndex)));
        outFragColor = texColor * inColor;
    } else {
        // Solid Color
        outFragColor = inColor;
    }
}
