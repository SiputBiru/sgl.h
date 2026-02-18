#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) flat in float inType;

layout(set = 2, binding = 0) uniform sampler2D tex0;
layout(set = 2, binding = 1) uniform sampler2D tex1;
layout(set = 2, binding = 2) uniform sampler2D tex2;
layout(set = 2, binding = 3) uniform sampler2D tex3;
layout(set = 2, binding = 4) uniform sampler2D tex4;
layout(set = 2, binding = 5) uniform sampler2D tex5;
layout(set = 2, binding = 6) uniform sampler2D tex6;
layout(set = 2, binding = 7) uniform sampler2D tex7;

layout(location = 0) out vec4 outFragColor;

void main() {
    int type = int(inType);

    if (type < 10 || type == 100) {
        if (type == 2) { // Circle
            if (length(inUV - 0.5) > 0.5) discard;
        }
        outFragColor = inColor;
    } else {
        int idx = type - 10;
        vec4 texC = vec4(1,0,1,1);
        switch(idx) {
            case 0: texC = texture(tex0, inUV); break;
            case 1: texC = texture(tex1, inUV); break;
            case 2: texC = texture(tex2, inUV); break;
            case 3: texC = texture(tex3, inUV); break;
            case 4: texC = texture(tex4, inUV); break;
            case 5: texC = texture(tex5, inUV); break;
            case 6: texC = texture(tex6, inUV); break;
            case 7: texC = texture(tex7, inUV); break;
        }
        outFragColor = texC * inColor;
    }
}
