#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma once

const float G = 6.6743e-11;

class CelestialBody {
public:
    glm::vec3 position;
    double gravityAcceleration;
    double averageRadius;
    double equatorialRadius;
    double polarRadius;
    double mass;

    glm::vec3 velocity;
    glm::vec3 sqrDstVec;
    double sqrDst;
    glm::vec3 forceDir;
    glm::vec3 acceleration;

    CelestialBody(glm::vec3 position, glm::vec3 velocity, double averageRadius, double equatorialRadius, double polarRadius, double gravityAcceleration) {
        this->position.x = position.x*1000.0f; // convert to meters (positions and velocities in kilometers, 1 unit = 1 meter, convert to units by multiplying by 1000)
        this->position.y = position.y*1000.0f; // swap Z and Y coordinates because of Horizons' Z up coordinate
        this->position.z = position.z*1000.0f;
        this->velocity = velocity*1000.0f;
        this->averageRadius = averageRadius;
        this->equatorialRadius = equatorialRadius;
        this->polarRadius = polarRadius;
        this->gravityAcceleration = gravityAcceleration;
        this->mass = calculateMass();
    }

    double calculateMass() {
        return this->gravityAcceleration * this->averageRadius * this->averageRadius / G;
    }

    void updateVelocity(std::vector<CelestialBody> &bodies, float deltaTime) {
        for (CelestialBody &body : bodies) {
            if (&body == this) continue;

            this->sqrDstVec.x = (body.position.x - this->position.x) * (body.position.x - this->position.x);
            this->sqrDstVec.y = (body.position.y - this->position.y) * (body.position.y - this->position.y);
            this->sqrDstVec.z = (body.position.z - this->position.z) * (body.position.z - this->position.z);
            this->sqrDst = this->sqrDstVec.x + this->sqrDstVec.y + this->sqrDstVec.z;

            if (this->sqrDst == 0.0) continue;

            this->forceDir.x = (body.position.x - this->position.x) / std::sqrt(this->sqrDst);
            this->forceDir.y = (body.position.y - this->position.y) / std::sqrt(this->sqrDst);
            this->forceDir.z = (body.position.z - this->position.z) / std::sqrt(this->sqrDst);

            this->acceleration.x = this->forceDir.x * ((G * body.mass) / this->sqrDst);
            this->acceleration.y = this->forceDir.y * ((G * body.mass) / this->sqrDst);
            this->acceleration.z = this->forceDir.z * ((G * body.mass) / this->sqrDst);

            this->velocity += this->acceleration * deltaTime;
        }
    }

    void updatePosition(float deltaTime) {
        this->position += this->velocity * deltaTime;
    }
};