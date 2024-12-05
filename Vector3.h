#pragma once
struct Vector3 final {
    float x;
    float y;
    float z;

    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
};