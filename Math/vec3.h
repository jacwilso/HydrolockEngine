#ifndef VEC3_H
#define VEC3_H

class vec3 {
public:
    union
    {
        struct { float x, y, z; };
        struct { float r, g, b; };
        float data[3];
    };

    vec3();
    vec3(float val);
    vec3(float x, float y, float z);
    vec3(float data[3]);

    vec3& operator=(const vec3& other);
    vec3& operator+=(const vec3& rhs);
    vec3& operator-=(const vec3& rhs);

    friend vec3 operator+(vec3 lhs, const vec3& rhs);
    friend vec3 operator-(vec3 lhs, const vec3& rhs);
    friend bool operator==(const vec3& lhs, const vec3& rhs);
};

#endif /* VEC3 */
