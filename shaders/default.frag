#version 450

// Matches Vertex Shader Output Location 0
layout(location = 0) in vec4 inColor;

// Matches Vertex Shader Output Location 1
layout(location = 1) in vec2 inUV;

// Matches Vertex Shader Output Location 2
layout(location = 2) flat in float inType; 

layout(location = 0) out vec4 outFragColor;

void main() {
    // Type 2.0 is SGL_SHAPE_CIRCLE
    // check > 1.5 to safely match "2" without floating point precision issues
    if (inType > 1.5) {
        // Circle Logic:
        // The UVs come in from 0.0 to 1.0. 
        // We calculate distance from the center (0.5, 0.5).
        vec2 uv = inUV - 0.5;
        if (length(uv) > 0.5) {
            discard; // Cut out the corners to make a circle
        }
    }

    outFragColor = inColor;
}
