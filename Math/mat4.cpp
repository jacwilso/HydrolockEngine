#include "mat4.h"

#include <cstring>
#include <cmath>

const mat4 mat4::Identity =  {1.0f, 0.0f, 0.0f, 0.0f, 
                              0.0f, 1.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f};

mat4::mat4()
{
    memset(data, 0, 16 * sizeof(float));
}

mat4::mat4(float m00, float m01, float m02, float m03, 
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33)
{
    this->m00 = m00;
    this->m01 = m01;
    this->m02 = m02;
    this->m03 = m03;

    this->m10 = m10;
    this->m11 = m11;
    this->m12 = m12;
    this->m13 = m13;

    this->m20 = m20;
    this->m21 = m21;
    this->m22 = m22;
    this->m23 = m23;

    this->m30 = m30;
    this->m31 = m31;
    this->m32 = m32;
    this->m33 = m33;
}

mat4::mat4(float data[16])
{
    memcpy(this->data, data, 16 * sizeof(float));
}

mat4& mat4::operator=(const mat4& other)
{
    memcpy(data, other.data, 16 * sizeof(float));
    return *this;
}

mat4& mat4::operator*=(const mat4& other)
{
    *this = *this * other;
    return *this;
}

mat4 operator*(mat4 lhs, const mat4& rhs)
{
    mat4 res;
    res.m00 = rhs.m00 * lhs.m00 + rhs.m01 * lhs.m10 + rhs.m02 * lhs.m20 + rhs.m03 * lhs.m30;
    res.m01 = rhs.m00 * lhs.m01 + rhs.m01 * lhs.m11 + rhs.m02 * lhs.m21 + rhs.m03 * lhs.m31;
    res.m02 = rhs.m00 * lhs.m02 + rhs.m01 * lhs.m12 + rhs.m02 * lhs.m22 + rhs.m03 * lhs.m32;
    res.m03 = rhs.m00 * lhs.m03 + rhs.m01 * lhs.m13 + rhs.m02 * lhs.m23 + rhs.m03 * lhs.m33;

    res.m10 = rhs.m10 * lhs.m00 + rhs.m11 * lhs.m10 + rhs.m12 * lhs.m20 + rhs.m13 * lhs.m30;
    res.m11 = rhs.m10 * lhs.m01 + rhs.m11 * lhs.m11 + rhs.m12 * lhs.m21 + rhs.m13 * lhs.m31;
    res.m12 = rhs.m10 * lhs.m02 + rhs.m11 * lhs.m12 + rhs.m12 * lhs.m22 + rhs.m13 * lhs.m32;
    res.m13 = rhs.m10 * lhs.m03 + rhs.m11 * lhs.m13 + rhs.m12 * lhs.m23 + rhs.m13 * lhs.m33;

    res.m20 = rhs.m20 * lhs.m00 + rhs.m21 * lhs.m10 + rhs.m22 * lhs.m20 + rhs.m23 * lhs.m30;
    res.m21 = rhs.m20 * lhs.m01 + rhs.m21 * lhs.m11 + rhs.m22 * lhs.m21 + rhs.m23 * lhs.m31;
    res.m22 = rhs.m20 * lhs.m02 + rhs.m21 * lhs.m12 + rhs.m22 * lhs.m22 + rhs.m23 * lhs.m32;
    res.m23 = rhs.m20 * lhs.m03 + rhs.m21 * lhs.m13 + rhs.m22 * lhs.m23 + rhs.m23 * lhs.m33;

    res.m30 = rhs.m30 * lhs.m00 + rhs.m31 * lhs.m10 + rhs.m32 * lhs.m20 + rhs.m33 * lhs.m30;
    res.m31 = rhs.m30 * lhs.m01 + rhs.m31 * lhs.m11 + rhs.m32 * lhs.m21 + rhs.m33 * lhs.m31;
    res.m32 = rhs.m30 * lhs.m02 + rhs.m31 * lhs.m12 + rhs.m32 * lhs.m22 + rhs.m33 * lhs.m32;
    res.m33 = rhs.m30 * lhs.m03 + rhs.m31 * lhs.m13 + rhs.m32 * lhs.m23 + rhs.m33 * lhs.m33;
    return res;
}

bool operator==(const mat4& lhs, const mat4& rhs)
{
    for (int i = 0; i < 16; ++i)
    {
        if (lhs.data[i] != rhs.data[i]) return false;
    }
    return true;
}

void mat4::transpose() // TODO: maybe can do this from a for loop?
{
    float tmp;

    tmp = m01;
    m01 = m10;
    m10 = tmp;

    tmp = m02;
    m02 = m10;
    m20 = tmp;

    tmp = m03;
    m03 = m30;
    m30 = tmp;

    tmp = m21;
    m21 = m12;
    m12 = tmp;

    tmp = m31;
    m31 = m13;
    m13 = tmp;

    tmp = m23;
    m23 = m32;
    m32 = tmp;
}

mat4 mat4::translate(float x, float y, float z)
{
    return {1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x,    y,    z,    1.0f};
}

mat4 mat4::eulerRotation(float roll, float yaw, float pitch)
{
    float cx = cos(roll);
    float sx = sin(roll);
    float cy = cos(yaw);
    float sy = sin(yaw);
    float cz = cos(pitch);
    float sz = sin(pitch);
    return {cy * cz, sx * sy * cz - cx * sz, cx * sy * sz - sx * cz, 0.0f,
            cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz, 0.0f,
            -sy,     sx * cy,                cx * cy,                0.0f,
            0.0f,    0.0f,                   0.0f,                   1.0f};
}

// CCW & Depth 0 to 1
mat4 mat4::projection(float fovy, float aspect, float znear, float zfar)
{
    float f = 1.0f / tan(0.5f * fovy);
    float z = 1.0f / (znear - zfar);
    return {f / aspect, 0.0f, 0.0f,             0.0f,
            0.0f,       -f,   0.0f,             0.0f,
            0.0f,       0.0f, zfar * z,         -1.0f,
            0.0f,       0.0f, znear * zfar * z, 0.0f};
}