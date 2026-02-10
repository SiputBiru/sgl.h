struct VSOutput {
    float4 pos : SV_Position;
    float4 color : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target0 {
    return input.color;
}
