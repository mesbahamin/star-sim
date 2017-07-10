#ifndef STAR_H
#define STAR_H

#include <math.h>
#include <stdint.h>

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

// TODO: Test whether making this function return a struct (rather than a
// struct pointer) makes a significant performance difference.
struct Vec2d *vec2d_add(float angle1, float length1, float angle2, float length2)
{
    float x = sinf(angle1) * length1 + sinf(angle2) * length2;
    float y = cosf(angle1) * length1 + cosf(angle2) * length2;

    struct Vec2d *new_vec = (struct Vec2d*)malloc(sizeof(struct Vec2d));
    new_vec->angle = 0.5 * M_PI - atan2f(y, x);
    new_vec->length = hypotf(x, y);
    return new_vec;
}

void star_accelerate(struct Star *s, float angle, float acceleration)
{
    struct Vec2d *new_vec = vec2d_add(s->angle, s->speed, angle, acceleration);
    s->angle = new_vec->angle;
    s->speed = new_vec->length;
    free(new_vec);
}

float star_calc_size(float mass)
{
    return 0.5 * (powf(mass, 0.5));
}

void star_attract(struct Star *s1, struct Star *s2)
{
    float dx = s1->x - s2->x;
    float dy = s1->y - s2->y;
    float distance = hypotf(dx, dy);

    if (distance > s1->size + s2->size)
    {
        float theta = atan2f(dy, dx);
        float force = GRAVITATION * (s1->mass * s2->mass / powf(distance, 2));
        star_accelerate(s1, theta - (0.5 * M_PI), force / s1->mass);
        star_accelerate(s2, theta + (0.5 * M_PI), force / s2->mass);
        //printf("%f\n", force);
    }
}

void star_attract_to_mass(struct Star *star, float mass, float mass_x, float mass_y)
{
    float dx = star->x - mass_x;
    float dy = star->y - mass_y;
    float distance = hypotf(dx, dy);

    float theta = atan2f(dy, dx);
    float force = GRAVITATION * (star->mass * mass / powf(distance, 2));
    star_accelerate(star, theta - (0.5 * M_PI), force / star->mass);
    //printf("%f\n", force);
}
#endif
