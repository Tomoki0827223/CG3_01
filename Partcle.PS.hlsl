#include "Particle.hlsl"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// 定数バッファでマテリアルとライティングを渡す
ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

PixelShaderOutput main(VertexShanderOutput input)
{
    // UV変換行列を使ってテクスチャ座標を変換
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    PixelShaderOutput output;

    if (gMaterial.enableLighting != 0)
    {
        // 照明計算（法線と光源方向の内積を用いた簡易ライティング）
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f); // コサイン補正
        output.color = gMaterial.color * textureColor * input.color;
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        // ライティング無しの場合の計算
        output.color = gMaterial.color * textureColor;
    }

    return output;
}
