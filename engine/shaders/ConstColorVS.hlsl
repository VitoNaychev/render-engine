struct VSInput {
    float2 position : POSITION;
};

cbuffer RootConstants : register(b0) {
    int frameIdx;
}

struct PSInput {
    float4 position : SV_POSITION;
};

PSInput VSMain(VSInput inputVertex) {
    float angle = 2 * 3.14 * frameIdx / 120;

    float x = inputVertex.position.x;
    float y = inputVertex.position.y;

    float cosA = cos(angle);
    float sinA = sin(angle);

    float2 rotated;
    rotated.x = x * cosA - y * sinA;
    rotated.y = x * sinA + y * cosA;

    return PSInput(float4(rotated, 0.0, 1.0));
}
