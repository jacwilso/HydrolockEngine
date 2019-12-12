#include "vec2.h"

#include <cstring>

vec2::vec2() : x(0), y(0) 
{
}

vec2::vec2(float val) : x(val), y(val)
{
}

vec2::vec2(float x, float y) : x(x), y(y)
{
}

vec2::vec2(float data[2])
{
    memcpy(data, this->data, 2 * sizeof(float));
}

vec2& vec2::operator=(const vec2& other)
{
    memcpy(data, other.data, 2 * sizeof(float));
    return *this;
}

vec2& vec2::operator+=(const vec2& rhs)
{
    x += rhs.x;
    y += rhs.y;
    return *this;
}

vec2& vec2::operator-=(const vec2& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

vec2 operator+(vec2 lhs, const vec2& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

vec2 operator-(vec2 lhs, const vec2& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

bool operator==(const vec2& lhs, const vec2& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}