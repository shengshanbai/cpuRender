#include "geometry.h"

vec2i_t vec2i_sub(vec2i_t v0, vec2i_t v1) {
    vec2i_t v;
    v.x = v0.x - v1.x;
    v.y = v0.y - v1.y;
    return v;
}