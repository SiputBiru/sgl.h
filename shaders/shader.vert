#version 450

struct InstanceData {
    vec4 rect;
    vec4 color;
};

layout(set = 0, binding = 0, std430) readonly buffer InstanceBuffer {
    InstanceData instances[];
} storage;

layout(set = 1, binding = 0, std140) uniform UniformBlock {
    vec2 ScreenSize;
    uint _pad;
} uniforms;


layout(location = 0) out vec4 v_Color;

void main()
{
    InstanceData data = storage.instances[gl_InstanceIndex ];

    vec2 corner = vec2(
      (gl_VertexIndex == 2 || gl_VertexIndex == 3 || gl_VertexIndex == 5),
      (gl_VertexIndex == 1 || gl_VertexIndex == 4 || gl_VertexIndex == 5)
    );


    vec2 worldPos = data.rect.xy + (corner * data.rect.zw);

    vec2 ndc;
    ndc.x = (worldPos.x / uniforms.ScreenSize.x) * 2.0 - 1.0;
    ndc.y = -((worldPos.y / uniforms.ScreenSize.y) * 2.0 - 1.0);

    gl_Position = vec4(ndc, 0.0, 1.0);
    v_Color = data.color;
}
