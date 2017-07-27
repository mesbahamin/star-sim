#ifndef STAR_H
#define STAR_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif

#define GRAVITATION 0.1f

struct Star
{
    float angle;
    float speed;
    float mass;
    float size;
    float x;
    float y;
    uint32_t color;
};

struct Vec2d
{
    float angle;
    float length;
};

struct Vec2d *vec2d_add(float angle1, float length1, float angle2, float length2);
void star_accelerate(struct Star *s, float angle, float acceleration);
float star_calc_size(float mass);
void star_attract(struct Star *s1, struct Star *s2);
void star_attract_to_mass(struct Star *star, float mass, float mass_x, float mass_y);

#endif
