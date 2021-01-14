#include "geometry.h"
#include <cmath>

vec2i_t vec2i_new(int x, int y) {
    vec2i_t v;
    v.x = x;
    v.y = y;
    return v;
}

vec3i_t vec3i_new(int x, int y, int z) {
    vec3i_t v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

vec2f_t vec2f_new(float x, float y) {
    vec2f_t v;
    v.x = x;
    v.y = y;
    return v;
}

vec3f_t vec3f_new(float x, float y, float z) {
    vec3f_t v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

vec2i_t vec2i_sub(vec2i_t v0, vec2i_t v1) {
    vec2i_t v;
    v.x = v0.x - v1.x;
    v.y = v0.y - v1.y;
    return v;
}

vec3f_t vec3f_cross(vec3f_t a, vec3f_t b) {
    vec3f_t result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

float vec3f_dot(vec3f_t a, vec3f_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3f_t vec3f_normalize(vec3f_t v) {
    float length = (float)sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return vec3f_new(v.x / length, v.y / length, v.z / length);
}

vec3f_t vec3f_sub(vec3f_t a, vec3f_t b) {
    return vec3f_new(a.x - b.x, a.y - b.y, a.z - b.z);
}