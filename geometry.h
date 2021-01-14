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
