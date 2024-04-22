#pragma once

#include <Windows.h>
#include <DirectXMath.h>

using namespace DirectX;

class MathHelper {
public:
    static XMFLOAT4X4 Identity4x4()
    {
        static XMFLOAT4X4 I(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

    static XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
    {
        return XMVectorSet(
            radius * sinf(phi) * cosf(theta),
            radius * cosf(phi),
            radius * sinf(phi) * sinf(theta),
            1.0f);
    }

    template<typename T>
    static T Clamp(const T& x, const T& low, const T& high) {
        return x < low ? low : (x > high ? high : x);
    }

    template<typename T>
    static T Max(const T& a, const T& b) {
        return a > b ? a : b;
    }

    // Returns random float in [0, 1).
    static float RandF()
    {
        return (float)(rand()) / (float)RAND_MAX;
    }

    // Returns random float in [a, b).
    static float RandF(float a, float b)
    {
        return a + RandF() * (b - a);
    }

    static int Rand(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }



    static const float Pi;

};