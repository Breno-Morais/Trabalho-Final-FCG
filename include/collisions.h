#ifndef _COLLISIONS_H
#define _COLLISIONS_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Raio
{
    glm::vec4 origem;
    glm::vec4 dir;
};

struct Esfera
{
    glm::vec4 centro;
    float r;
};

class Cubo
{
public:
    glm::vec4 vert_min;
    glm::vec4 vert_max;

    glm::vec4 centro()
    {
        return (vert_min + vert_max)/2.0f;
    }

    void newCentro(glm::vec4 newPos)
    {
        vert_min = newPos + (vert_min - centro());
        vert_max = newPos + (vert_max - centro());
    }
};

struct Plano
{
    glm::vec4 n; // Normal
    float d; // Distancia da origem
};

struct Cubo_Collision
{
    Cubo cube;
    bool colide;
    glm::mat4 Matrix_Model;
    std::string objName;
    bool visto;
    float tempoVisto;
    float t;
    glm::vec3 Path[4];
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    glm::vec3 Ke;
};

struct Sphere_Collision
{
    Esfera bola;
    bool colide;
    float scale;
    std::string objName;
    bool visto;
    float tempoVisto;
    float t;
    glm::vec3 Path[4];
};

float collision_Ray_Sphere(Raio ray, Esfera sphere);
bool collision_Box_Box(Cubo box1, Cubo box2);
bool collision_Ray_Box(Raio ray, Cubo box);
bool collision_Box_Plane(Cubo box1, Plano p);
bool collision(Raio ray, Esfera sphere);
bool collision(Cubo box1, Cubo box2);
bool collision(Raio ray, Cubo box);
bool collision(Cubo box1, Plano p);

#endif
