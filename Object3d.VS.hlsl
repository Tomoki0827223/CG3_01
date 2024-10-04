#include "Object3d.hlsl"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 world;

};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
};


VertexShanderOutput main(VertexShaderInput input)
{
  
    VertexShanderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.world));
    
    return output;
};