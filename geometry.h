#pragma once

typedef struct {
    float x, y;
} vec2f_t;

typedef struct {
    float x, y, z;
} vec3f_t;

typedef struct {int x, y;} vec2i_t;
typedef struct {int x, y, z;} vec3i_t;

vec2i_t vec2i_sub(vec2i_t v0, vec2i_t v1);

vec3f_t vec3f_cross(vec3f_t a, vec3f_t b);

float vec3f_dot(vec3f_t a, vec3f_t b);

vec3f_t vec3f_normalize(vec3f_t v);
vec3f_t vec3f_sub(vec3f_t a, vec3f_t b);

vec2i_t vec2i_new(int x, int y);
vec3i_t vec3i_new(int x, int y, int z);
vec2f_t vec2f_new(float x, float y);
vec3f_t vec3f_new(float x, float y, float z);