#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "DirectXCommon.h"
#include "D3DResourceLeakChecker.h"
#include "Input.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <string>
#include <random>
#include <numbers>
#include "affine.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

#pragma region 単位行列とTransform
// 単位行列の作成
Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; ++i) {
		result.m[i][i] = 1;
	}
	return result;
}

struct TransformVector3
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;

};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 nomal;
};

struct Material
{
	Vector4 color;
	int32_t endleLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct Particle
{
	TransformVector3 transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 world;
	Vector4 color;
};


std::random_device seedGenerator;
std::mt19937 randomEngine(seedGenerator());

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 world;

};

struct DirectionaLight
{
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct Transform1 {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

Transform1 uvTransformSprite{
	{1.0f, 1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f},
};

struct MaterialData {
	std::string textureFilePath;
};


struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

#pragma endregion

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	// 1.2.必要な変数の宣言とファイルを開く
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	// 3.ファイルを読み、MaterialDataを構築
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData LoaObjFile(const std::string& directoryPath, const std::string& filename) {
	
	// 1. 中で必要となる変数の宣言
	ModelData modelData; // 構築するModalData
	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line; // ファイルから読んだ1行を格納するもの
	
	// 2. ファイルを開く
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	
	// 3. 実際のファイルを読み込み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む
		// identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;

			position.x *= -1.0f;

			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;

			texcoord.y = 1.0f - texcoord.y;
			
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;

			s >> normal.x >> normal.y >> normal.z;

			normal.x *= -1.0f;

			normals.push_back(normal);
		}
		else if (identifier == "f") {
			
			VertexData triangle[3];
			
			// 面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndeices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndeices[element] = std::stoi(index);
				}

				// 要素へのIndexから、実際の要素の値をを取得して頂点を構築する
				Vector4 position = positions[elementIndeices[0] - 1];
				Vector2 texcoord = texcoords[elementIndeices[1] - 1];
				Vector3 normal = normals[elementIndeices[2] - 1];
				VertexData vertex = { position,texcoord,normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position,texcoord,normal };
			}

			// 頂点を逆順で登録することで、回り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {
			// mateialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;

}

Particle MakeNewParticle(std::mt19937& randomEngine)
{
	std::uniform_real_distribution<float> distribution(-0.2f, 0.2f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> destTime(1.0f, 8.0f);
	
	Particle particle;
	
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };
	particle.velocity = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };
	particle.color = { distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), 1.0f };
	particle.lifeTime = destTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	//D3DResourceLeakChecker LeakCheak;

	WinApp* winApp_ = nullptr;
	winApp_ = new WinApp();
	winApp_->Initialize();

#pragma region Windowの生成

	//GE3
	Input* input = nullptr;
	input = new Input();
	input->Initialize(winApp_);

	DirectXCommon* dxCommon = nullptr;
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp_);

	//ウインドウを表示する
	ShowWindow(winApp_->GetHwnd(), SW_SHOW);

#pragma endregion

#pragma region  descriptionRootSignature

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0;
	descriptorRangeForInstancing[0].NumDescriptors = 1;
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//RootParameter作成。複数設定できるので配列。今回は結果は1つだけなので長さ１の配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	//koko
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);
	
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);


	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(hr))
	{
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);

	}
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
#pragma endregion


#pragma region inputElementDescとgraphicsPinpelineStateDesc

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = true;
	
	////通常
	//blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	//加算
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	

	//変更しないもの
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//裏面表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlobComPtr = dxCommon->compileShader(L"resources/shaders/Object3d.VS.hlsl", L"vs_6_0");
	IDxcBlob* vertexShaderBlob = vertexShaderBlobComPtr.Get();
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlobComPtr = dxCommon->compileShader(L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0");
	IDxcBlob* pixelShaderBlob = pixelShaderBlobComPtr.Get();
	assert(pixelShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> vertexParticleShaderBlobComPtr = dxCommon->compileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
	IDxcBlob* vertexParticleShaderBlob = vertexParticleShaderBlobComPtr.Get();
	assert(vertexParticleShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderParticleBlobComPtr = dxCommon->compileShader(L"resources/shaders/Partcle.PS.hlsl", L"ps_6_0");
	IDxcBlob* pixelShaderParticleBlob = pixelShaderParticleBlobComPtr.Get();
	assert(pixelShaderParticleBlob != nullptr);


	//ここから03_01
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	//ここから03_01

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPinpelineStateDesc{};
	graphicsPinpelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPinpelineStateDesc.InputLayout = inputLayoutDesc;
	/*graphicsPinpelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),*/
	graphicsPinpelineStateDesc.VS = { vertexParticleShaderBlob->GetBufferPointer(),
		vertexParticleShaderBlob->GetBufferSize() };
		vertexShaderBlob->GetBufferSize();
	graphicsPinpelineStateDesc.PS = { pixelShaderParticleBlob->GetBufferPointer(),pixelShaderParticleBlob->GetBufferSize() };
	graphicsPinpelineStateDesc.BlendState = blendDesc;
	graphicsPinpelineStateDesc.RasterizerState = rasterizerDesc;

	graphicsPinpelineStateDesc.NumRenderTargets = 1;
	graphicsPinpelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	graphicsPinpelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsPinpelineStateDesc.SampleDesc.Count = 1;
	graphicsPinpelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	graphicsPinpelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPinpelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResouce = dxCommon->CreateDepthStencilTextureResource(dxCommon->GetDevice(), WinApp::kClientWidth, WinApp::kClientHeight);

	//ここから03_01
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dxCommon->GetDevice()->CreateDepthStencilView(depthStencilResouce.Get(), &dsvDesc, dxCommon->GetDSVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
	//ここから03_01

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPinpelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));


#pragma endregion

	//Material用のResourceを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	Material* materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	////こここで色かえられるよ
	materialData->color = { 1.0f,1.0f,1.0f,1.0f };
	materialData->endleLighting = true;
	materialData->uvTransform = MakeIdentity4x4();

	//パーティクル最大数
	const uint32_t kNumMaxInstance = 50;
	
	
	//Material用のResourceを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = dxCommon->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
	ParticleForGPU* instancingData = nullptr;
	instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));
	////こここで色かえられるよ

	for (uint32_t index = 0; index < kNumMaxInstance; index++)
	{
		instancingData[index].WVP = MakeIdentity4x4();
		instancingData[index].world = MakeIdentity4x4();
		instancingData[index].color = Vector4{ 1.0f,1.0f,1.0f,1.0f };
	}

	Particle particles[kNumMaxInstance];
	for (uint32_t index = 0; index < kNumMaxInstance; index++)
	{
		particles[index] = MakeNewParticle(randomEngine);
	}


	//bool useUpdate = false;
	bool useMonsterBall = false;
	
	const float kDeltaTime = 1.0f / 60.0f;

	TransformVector3 transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Matrix4x4 backToFontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);

	//モデル読み込み
	//ModelData modelData = LoaObjFile("resources", "Bunny.obj");
	ModelData modelData = LoaObjFile("resources", "plane.obj");
	
	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();// リソースの先頭のアドレスから使う
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());// 使用するリソースのサイズは頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);// 1頂点あたりのサイズ

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));// 書き込むためのアドレスを取得
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData)* modelData.vertices.size());// 頂点データをリソースにコピー

	//DepthStencilTextureを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = dxCommon->CreateDepthStencilTextureResource(dxCommon->GetDevice(), WinApp::kClientWidth, WinApp::kClientHeight);

	//VertexBufferResourceを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;

	//Sprite用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = dxCommon->CreateBufferResource(sizeof(Material));
	Material* materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDataSprite->endleLighting = false;
	materialDataSprite->uvTransform = MakeIdentity4x4();

	//ライティング
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResorce = dxCommon->CreateBufferResource(sizeof(DirectionaLight));
	DirectionaLight* directionalLightData = nullptr;
	directionalLightResorce->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;

	//TransformationMatrixResource
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixDataSprite = nullptr;
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	transformationMatrixDataSprite->WVP = MakeIdentity4x4();
	transformationMatrixDataSprite->world = MakeIdentity4x4();

	// Sprite用のTransfomationMatrix用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpDeta = nullptr;
	// 書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpDeta));
	// 単位行列を書き込んでおく
	wvpDeta->world = MakeIdentity4x4();
	wvpDeta->WVP = MakeIdentity4x4();

	////vetexResourceSprite頂点バッファーを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{ };
	//リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//1頂点当たりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);
	//頂点リソースにデータを書き込む
	VertexData* vertexDataSprite = nullptr;
	//書き込むためのアドレスを取得
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	// 1枚目の三角形
	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };// 左下
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[0].nomal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };// 左上
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[1].nomal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };// 右下
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[2].nomal = { 0.0f, 0.0f, -1.0f };
	// 2枚目の三角形
	vertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };// 右上
	vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
	vertexDataSprite[3].nomal = { 0.0f, 0.0f, -1.0f };


	TransformVector3 transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	TransformVector3 cameraTransform
	{
		{1.0f,1.0f,1.0f},
		{std::numbers::pi_v<float> / 4.0f,std::numbers::pi_v<float> , 0.0f},
		{0.0f,5.0f,5.0f}
	};

	//TransformVector3 cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };
	//Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	dxCommon->GetCPUDescriptorHandle(dxCommon->GetRTVDescriptorHeap(), dxCommon->GetDescriptorSizeRTV(), 0);


	DirectX::ScratchImage mipimage2 = dxCommon->LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipimage2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metadata2);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResources2 = dxCommon->UploadTextureData(textureResource2, mipimage2);

	//metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	//SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = dxCommon->GetCPUDescriptorHandle(dxCommon->GetSRVDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = dxCommon->GetGPUDescriptorHandle(dxCommon->GetSRVDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 2);
	//SRVの生成
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);


	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU = dxCommon->GetCPUDescriptorHandle(dxCommon->GetSRVDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 3);
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU = dxCommon->GetGPUDescriptorHandle(dxCommon->GetSRVDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 3);
	dxCommon->GetDevice()->CreateShaderResourceView(instancingResource.Get(), &instancingSrvDesc, instancingSrvHandleCPU);


	//DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	DirectX::ScratchImage mipImages2 = dxCommon->LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResources = dxCommon->UploadTextureData(textureResource, mipImages2);
	//metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	
	//SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetSRVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = dxCommon->GetSRVDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();

	//戦闘はImGuiが使っているのでその次を使う
	textureSrvHandleCPU.ptr += dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//SRVの生成
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

	bool useBillboard = false;

	MSG msg{};

	dxCommon->InitializeImGui();



	while (true)
	{
		if (winApp_->ProsessMeassage())
		{
			break;
		}
		else
		{
			// GE3
			input->Update();
			// ゲーム処理

			// 描画前処理
			dxCommon->PreDraw();


			transform.rotate.y += 0.0f;
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(winApp_->kClientWidth) / float(winApp_->kClientHeight), 0.1f, 100.0f);
			
			//Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

			Matrix4x4 billboardMatrix = MakeIdentity4x4();
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;

			Matrix4x4 translationMatrix = MakeTranslateMatrix(transform.translate);
			Matrix4x4 ScaleMatrix = MakeScaleMatrix(transform.scale);
			
			
			Matrix4x4 worldMatrix = ScaleMatrix * billboardMatrix * translationMatrix;


			//Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
			//wvpDeta->world = worldMatrix;
			//wvpDeta->WVP = worldViewProjectionMatrix;

			//Matrix4x4 worldMatrix = Multiply(ScaleMatrix, Multiply(billboardMatrix, translationMatrix));
			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, projectionMatrix);
			wvpDeta->world = worldMatrix;
			wvpDeta->WVP = worldViewProjectionMatrix;


			// Sprite用のWorldViewProjectionMatrixを作る
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(winApp_->kClientWidth), float(winApp_->kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
			transformationMatrixDataSprite->world = worldMatrixSprite;
			transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;


			//スプライト
			Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataSprite->uvTransform = uvTransformMatrix;

			uint32_t numInstance = 0; // 描画すべきインスタンス数

			for (uint32_t index = 0; index < kNumMaxInstance; ++index) {
				if (particles[index].lifeTime <= particles[index].currentTime) { // 生存期間を過ぎていたら更新せず描画対象にしない
					continue;
				}

				// ワールド行列を作成

				float alpha = 1.0f - (particles[index].currentTime / particles[index].lifeTime);

				Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
				Matrix4x4 worldMatrix = MakeAffineMatrix(particles[index].transform.scale, particles[index].transform.rotate, particles[index].transform.translate);
				Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
				//instancingData[index].WVP = worldViewProjectionMatrix,
				//instancingData[index].world = worldMatrix;
				//instancingData[index].color = particles[index].color;
				
				particles[index].transform.translate.x += particles[index].velocity.x * kDeltaTime;
				particles[index].transform.translate.y += particles[index].velocity.y * kDeltaTime;
				particles[index].transform.translate.z += particles[index].velocity.z * kDeltaTime;
				particles[index].currentTime += kDeltaTime; // 経過時間を足す
				
				instancingData[numInstance].WVP = worldViewProjectionMatrix; 
				instancingData[numInstance].world = worldMatrix;
				instancingData[numInstance].color = particles[index].color;
				instancingData[numInstance].color.w = alpha;
				
				++numInstance; // 生きているParticleの数を1つカウントする

                particles[index].transform.translate += Vector3(particles[index].velocity.x * kDeltaTime, particles[index].velocity.y * kDeltaTime, particles[index].velocity.z * kDeltaTime);

			}


			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Color");
			ImGui::ColorEdit4("Text Color With Flags", &materialData->color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			ImGui::End();

			// ImGuiウィンドウの作成
			ImGui::Begin("Ball Controls");
			ImGui::SliderFloat3("ParticleX", &particles->transform.rotate.x, -5.0f, 5.0f);
			ImGui::SliderFloat3("ParticleY", &particles->transform.rotate.y, -180.0f, 180.0f);
			//ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 2.0f);

			ImGui::Checkbox("useUVTexture", &useMonsterBall);

			ImGui::Checkbox("Billboard", &useBillboard);

			if (useBillboard) {
				Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
				billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);
				billboardMatrix.m[3][0] = 0.0f;
				billboardMatrix.m[3][1] = 0.0f;
				billboardMatrix.m[3][2] = 0.0f;
			}



			ImGui::End();
			// 他の描画処理が完了した後に ImGui 描画を行う
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

			dxCommon->InitializeViewportAndScissorRect();
			dxCommon->InitializeScissorRect();


			//RootSignatureを設定。PSOに設定しているけど別途設定が必要
			dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
			dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

			//Sphere
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
			//形状を設定。PSOに設定しているものとはまた別、同じものを設定すると考えておけば良い
			dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			//wvp用のCBufferの場所を設定
			//commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResorce->GetGPUVirtualAddress());

			// 他の設定諸々
			// instancing用のDataを読むためにStructured Buffer SRVを設定する
			dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU);
			// 他の設定諸々
			//描画! 6頂点の板ポリゴンを、kNumInstance(今回は10)だけInstance描画を行う
			dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), numInstance, 0, 0);

			// 描画後処理
			dxCommon->PostDraw();
		}
	}
	//Windows終了
	winApp_->Finalize();

	delete input;
	delete winApp_;
	delete dxCommon;

	return 0;
}