#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 direction, float speed, float focalLength, float fov, glm::vec3 worldUp) {
	this->position = position;
	this->direction = direction;
	this->speed = speed;
	this->focalLength = focalLength;
	this->fov = fov;
	this->worldUp = worldUp;

	yaw = -90.0f;
	pitch = 0.0f;

	updateVectors();
}

void Camera::Move(int dir_enum, float deltaTime) {
	switch (dir_enum) {
	case FOWARD:
		position += direction * speed * deltaTime;
		break;
	case BACKWARD:
		position -= direction * speed * deltaTime;
		break;
	case UP:
		position += worldUp * speed * deltaTime;
		break;
	case DOWN:
		position -= worldUp * speed * deltaTime;
		break;
	case LEFT:
		position -= rightv * speed * deltaTime;
		break;
	case RIGHT:
		position += rightv * speed * deltaTime;
		break;
	}
}

void Camera::Rotate(float dx, float dy, float deltaTime) {
	yaw += dx * 0.1f;
	pitch -= dy * 0.1f;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	updateVectors();
}

void Camera::Zoom(float d, float deltaTime) {
	fov -= d;
	if (fov < 0.0f) {
		fov = 0.0f;
	}
	if (fov > 179.0f) {
		fov = 179.0f;
	}
}

void Camera::updateVectors() {
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction = glm::normalize(direction);

	rightv = glm::normalize(glm::cross(direction, worldUp));
	upv = glm::normalize(glm::cross(rightv, direction));
}

glm::vec3 Camera::getLookingAt() {
	return position + direction * focalLength;
}

void Camera::LookAt(glm::vec3 pos) {
	direction = glm::normalize(pos - position);
	rightv = glm::normalize(glm::cross(direction, worldUp));
	upv = glm::normalize(glm::cross(rightv, direction));
}