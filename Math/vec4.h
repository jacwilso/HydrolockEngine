#ifndef VEC4_H
#define VEC4_H

class vec4 {
public:
    union
    {
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
        float data[4];
    };

    vec4();
    vec4(float val);
    vec4(float x, float y, float z, float w);
    vec4(float data[4]);

    vec4& operator=(const vec4& other);
    vec4& operator+=(const vec4& rhs);
    vec4& operator-=(const vec4& rhs);

    friend vec4 operator+(vec4 lhs, const vec4& rhs);
    friend vec4 operator-(vec4 lhs, const vec4& rhs);
    friend bool operator==(const vec4& lhs, const vec4& rhs);
};

#endif /* VEC3 */
