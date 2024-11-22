#include "Object3d.hlsl"

struct ParticleForGPU
{
    float32_t4x4 WVP;
    float32_t4x4 world;
    float32_t4 color;

};
ConstantBuffer<ParticleForGPU> gParticle : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color : COLOR0;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
};


VertexShanderOutput main(VertexShaderInput input)
{
  
    VertexShanderOutput output;
    output.position = mul(input.position, gParticle.WVP);
    output.texcoord = input.texcoord;
    output.color = gParticle[InstanceID].color;
    
    return output;
};