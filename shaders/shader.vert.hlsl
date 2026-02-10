struct InstanceData {
    float4 rect;  // x, y, w, h
    float4 color; // r, g, b, a
    // padding was handled in C struct
};

// Binding slot 0, space 0 matches SDL_BindGPUVertexStorageBuffers(..., 0, ...)
StructuredBuffer<InstanceData> Instances : register(t0, space0);

struct VSInput {
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput {
    float4 pos : SV_Position;
    float4 color : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    
    // 1. Fetch Data "Pulling"
    InstanceData data = Instances[input.instanceID];

    // 2. Generate Quad Vertices (0..5)
    // 0,1,2 (Tri 1) - 3,4,5 (Tri 2)
    float2 corner;
    // Simple logic to map 0-5 to unit quad corners
    uint idx = input.vertexID % 6;
    if (idx == 0) corner = float2(0, 0);
    else if (idx == 1) corner = float2(0, 1);
    else if (idx == 2) corner = float2(1, 0);
    else if (idx == 3) corner = float2(1, 0);
    else if (idx == 4) corner = float2(0, 1);
    else                 corner = float2(1, 1);

    // 3. Apply position/size from instance data
    float2 worldPos = data.rect.xy + (corner * data.rect.zw);

    // 4. Convert to NDC (assuming 800x600 screen for this demo)
    // In a real lib, pass screen size in a UniformBuffer
    float2 ndc;
    ndc.x = (worldPos.x / 800.0) * 2.0 - 1.0;
    ndc.y = -((worldPos.y / 600.0) * 2.0 - 1.0); // Flip Y for Vulkan/SDL3

    output.pos = float4(ndc, 0.0, 1.0);
    output.color = data.color;
    
    return output;
}
