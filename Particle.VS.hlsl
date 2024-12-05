#include "Particle.hlsl"

struct ParticleForGPU
{
    float32_t4x4 WVP;
    float32_t4x4 world;
    float32_t4 color;
};


// StructuredBufferにインスタンスごとの行列を格納
StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};


VertexShanderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShanderOutput output;

    // InstanceIDを使ってインスタンスごとのWVPとWorld行列を取得
    output.position = mul(input.position, gParticle[instanceId].WVP);
    output.texcoord = input.texcoord;
    output.color = gParticle[instanceId].color;
    output.normal = normalize(mul(input.normal, (float3x3) gParticle[instanceId].world));

    return output;
}
