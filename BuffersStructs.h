#pragma once

#include <glm/glm.hpp>

#define BVH_TYPE_BVH 0
#define BVH_TYPE_SPHERE 1

struct alignas(16) CameraBuffer {
    glm::vec3 position; float __p;
    glm::vec3 viewportTopLeft; float __p2;
    glm::vec3 du; float __p3;
    glm::vec3 dv; float __p4;
    glm::vec3 backgroundColour; float __p5;
    glm::vec2 screenRes;
};

struct alignas(16) MaterialBuffer {
    glm::vec3 colour = glm::vec3(1, 1, 1);
    float reflective = 0.0f;
    float refractive = 0.0f;
    bool emitter = false;
    MaterialBuffer(glm::vec3 _colour = glm::vec3(1, 1, 1), float _reflective = 0.0f, float _refractive = 0.0f, bool _emitter = false) {
        colour = _refractive > 0.0f ? glm::vec3(1, 1, 1) : _colour;
        reflective = _reflective;
        refractive = _refractive;
        emitter = _emitter;
    }
};

struct alignas(16) SpheresBuffer {
    MaterialBuffer material;
    glm::vec3 position;
    float radius;

    SpheresBuffer(glm::vec3 _position, float _radius) {
        position = _position;
        radius = _radius;
    }
};


struct alignas(16) BVHBuffer {
    glm::vec3 AABBmin; float __p;
    glm::vec3 AABBmax;
    int left_index = -1;
    int right_index = -1;
    int type;
};



