#pragma once

struct Matrix4x4 final {
	float m[4][4];

	// 掛け算が必要
	Matrix4x4 operator*(const Matrix4x4& other) const {
		Matrix4x4 result = {};
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; ++k) {
					result.m[i][j] += m[i][k] * other.m[k][j];
				}
			}
		}
		return result;
	}

	// 平行移動行列
	Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
		return {
			1.0f, 0.0f, 0.0f, 0.0f, 
			0.0f, 1.0f, 0.0f, 0.0f, 
			0.0f, 0.0f, 1.0f, 0.0f,
			translate.x, translate.y, translate.z, 1.0f,
		};
	}

	// 拡大行列
	Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
		return { 
			scale.x, 0.0f, 0.0f, 0.0f, 
			0.0f, scale.y, 0.0f, 0.0f, 
			0.0f, 0.0f, scale.z, 0.0f, 
			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}
};