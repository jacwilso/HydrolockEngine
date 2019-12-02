#ifndef MAT4_H
#define MAT4_H

class mat4
{
public:
    union
    {
        struct
        {
            float m00, m01, m02, m03, 
                  m10, m11, m12, m13,
                  m20, m21, m22, m23,
                  m30, m31, m32, m33; 
        };
        float data[16];
    };

    mat4();
    mat4(float m00, float m01, float m02, float m03, 
         float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23,
         float m30, float m31, float m32, float m33);
    mat4(float data[16]);

    mat4& operator=(const mat4& other);
    mat4& operator*=(const mat4& other);

    friend mat4 operator*(mat4 lhs, const mat4& rhs);
    friend bool operator==(const mat4& lhs, const mat4& rhs);

    void transpose();

    const static mat4 Identity;

    static mat4 translate(float x, float y, float z);
    static mat4 eulerRotation(float yaw, float pitch, float roll);
    static mat4 projection(float fovy, float aspect, float znear, float zfar);
};

#endif /* MAT4_H */