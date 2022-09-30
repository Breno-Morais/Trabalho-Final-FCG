#include "collisions.h"

template <typename T>
float norm(T v)
{
    float vx = v.x;
    float vy = v.y;
    float vz = v.z;

    return sqrt( vx*vx + vy*vy + vz*vz );
}

float dotproductp(glm::vec4 u, glm::vec4 v)
{
    float u1 = u.x;
    float u2 = u.y;
    float u3 = u.z;
    float u4 = u.w;
    float v1 = v.x;
    float v2 = v.y;
    float v3 = v.z;
    float v4 = v.w;

    if ( u4 != 0.0f || v4 != 0.0f )
    {
        fprintf(stderr, "ERROR: Produto escalar não definido para pontos.\n");
        std::exit(EXIT_FAILURE);
    }

    return u1*v1 + u2*v2 + u3*v3;
}

float collision_Ray_Sphere(Raio ray, Esfera sphere)
{
    float a, b, c, x1, x2, delta;
    a = pow(norm(ray.dir),2);
    b = dotproductp(2.0f*ray.dir, (ray.origem - sphere.centro));
    c = pow(norm(ray.origem - sphere.centro),2) - pow(sphere.r, 2);
    delta = b*b - 4*a*c;

    if (delta > 0) {
        x1 = (-b + sqrt(delta)) / (2*a);
        x2 = (-b - sqrt(delta)) / (2*a);
        if(x1 < 0 || x2 < 0) return -1.0f;
        return std::min(x1,x2);
    }

    else if (delta == 0) {
        x1 = -b/(2*a);
        return x1;
    }

    return -1.0f;
}

bool collision_Box_Box(Cubo box1, Cubo box2)
{
    return   (box1.vert_min.x <= box2.vert_max.x && box1.vert_max.x >= box2.vert_min.x) &&
             (box1.vert_min.y <= box2.vert_max.y && box1.vert_max.y >= box2.vert_min.y) &&
             (box1.vert_min.z <= box2.vert_max.z && box1.vert_max.z >= box2.vert_min.z);
}

bool collision_Ray_Box(Raio ray, Cubo box)
{
    float tmin = (box.vert_min.x - ray.origem.x) / ray.dir.x;
    float tmax = (box.vert_max.x - ray.origem.x) / ray.dir.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (box.vert_min.y - ray.origem.y) / ray.dir.y;
    float tymax = (box.vert_max.y - ray.origem.y) / ray.dir.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (box.vert_min.z - ray.origem.z) / ray.dir.z;
    float tzmax = (box.vert_max.z - ray.origem.z) / ray.dir.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    return true;
}  // FONTE http://www.realtimerendering.com/intersections.html

bool collision_Box_Plane(Cubo box1, Plano p)
{
    glm::vec4 c = box1.centro();
    glm::vec4 e = box1.vert_max - c;

    float r = e.x*abs(p.n.x) + e.y*abs(p.n.y) + e.z*abs(p.n.z);

    float s = dotproductp(p.n, glm::vec4(c.x, c.y, c.z, 0.0f)) - p.d;

    return abs(s) <= r;
} // FONTE http://www.realtimerendering.com/intersections.html

bool collision(Raio ray, Esfera sphere)
{
    float t = collision_Ray_Sphere(ray,sphere);
    if(t < 0)
        return false;
    else
        return true;
}

bool collision(Cubo box1, Cubo box2)
{
    return collision_Box_Box(box1,box2);
}

bool collision(Raio ray, Cubo box)
{
    return collision_Ray_Box(ray, box);
}

bool collision(Cubo box1, Plano p)
{
    return collision_Box_Plane(box1, p);
}
