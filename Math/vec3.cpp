#include "vec3.h"

#include <cstring>

vec3::vec3() : x(0), y(0), z(0)
{
}

vec3::vec3(float val) : x(val), y(val), z(val)
{
}

vec3::vec3(float x, float y, float z) : x(x), y(y), z(z)
{
}

vec3::vec3(float data[3])
{
    memcpy(data, this->data, 3 * sizeof(float));
}

vec3& vec3::operator=(const vec3& other)
{
    memcpy(data, other.data, 3 * sizeof(float));
    return *this;
}

vec3& vec3::operator+=(const vec3& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

vec3& vec3::operator-=(const vec3& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

vec3 operator+(vec3 lhs, const vec3& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

vec3 operator-(vec3 lhs, const vec3& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
}

bool operator==(const vec3& lhs, const vec3& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}