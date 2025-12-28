struct PSInput {
    float4 position : SV_POSITION;
    uint primitiveID : SV_PrimitiveID;
};

float4 PSMain(PSInput input) : SV_TARGET {
    float4 colors[6] = {
        float4(1.0, 0.0, 0.0, 1.0),  // Red
        float4(0.0, 1.0, 0.0, 1.0),  // Green
        float4(0.0, 0.0, 1.0, 1.0),  // Blue
        float4(1.0, 1.0, 0.0, 1.0),  // Yellow
        float4(1.0, 0.0, 1.0, 1.0),  // Magenta
        float4(0.0, 1.0, 1.0, 1.0),  // Cyan
    };
    return colors[input.primitiveID];
}
