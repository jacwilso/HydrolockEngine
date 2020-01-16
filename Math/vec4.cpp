#include "vec4.h"

#include <cstring>

vec4::vec4() : x(0), y(0), z(0)
{
}

vec4::vec4(float val) : x(val), y(val), z(val), w(val)
{
}

vec4::vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
{
}

vec4::vec4(float data[4])
{
    memcpy(data, this->data, 4 * sizeof(float));
}

vec4& vec4::operator=(const vec4& other)
{
    memcpy(data, other.data, 4 * sizeof(float));
    return *this;
}

vec4& vec4::operator+=(const vec4& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
}

vec4& vec4::operator-=(const vec4& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    w -= rhs.w;
    return *this;
}

vec4 operator+(vec4 lhs, const vec4& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    lhs.w += rhs.w;
    return lhs;
}

vec4 operator-(vec4 lhs, const vec4& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    lhs.w -= rhs.w;
    return lhs;
}

bool operator==(const vec4& lhs, const vec4& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}