#version 450
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) in flat int inType;

layout(location = 0) out vec4 outFragColor;

void main() {
    if (inType == 2) { // Circle Logic
        float dist = distance(inUV, vec2(0.5));
        if (dist > 0.5) discard;
    }
    outFragColor = inColor;
}
