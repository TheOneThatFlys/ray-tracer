#pragma once
#include <glm/glm.hpp>
#include "Utils.h"

class Camera
{
public:
	glm::vec3 position, direction, upv, rightv;
	float pitch, yaw;
	float focalLength;
	float fov;
	Camera(
		glm::vec3 position = glm::vec3(0, 0, 0),
		glm::vec3 direction = glm::vec3(0, 0, 1),
		float speed = 1.0f,
		float focalLength = 1.0f,
		float fov = 45.0f,
		glm::vec3 worldUp = glm::vec3(0, 1, 0)
	);
	void Move(int direction, float deltaTime);
	void Rotate(float dx, float dy, float deltaTime);
	void Zoom(float d, float deltaTime);
	glm::vec3 getLookingAt();
	void LookAt(glm::vec3 pos);
	void updateVectors();
private:
	float speed;
	glm::vec3 worldUp;
};

