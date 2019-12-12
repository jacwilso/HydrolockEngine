#ifndef VEC2_H
#define VEC2_H

class vec2 {
public:
    union 
    {
        struct { float x, y; };
        float data[2];
    };

    vec2();
    vec2(float val);
    vec2(float x, float y);
    vec2(float data[2]);

    vec2& operator=(const vec2& other);
    vec2& operator+=(const vec2& rhs);
    vec2& operator-=(const vec2& rhs);

    friend vec2 operator+(vec2 lhs, const vec2& rhs);
    friend vec2 operator-(vec2 lhs, const vec2& rhs);
    friend bool operator==(const vec2& lhs, const vec2& rhs);
};

#endif /* VEC2_H */
