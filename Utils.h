#pragma once

#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

enum Direction {
	UP, DOWN, FOWARD, BACKWARD, LEFT, RIGHT
};

static void checkGlErrors(std::string desc)
{
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		fprintf(stderr, "OpenGL error in \"%s\": (%d)\n", desc.c_str(), e);
		exit(20);
	}
}

static void glErrorCallback(int code, const char* desc) {
	std::cout << "OpenGL error: " << desc << "\n(Code " << code << ")\n";
	exit(20);
}

static void printVec3(glm::vec3 v) {
	std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
}

static std::string toString(glm::vec3 v) {
	return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

static bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start = str.find(from);
	if (start == std::string::npos) {
		return false;
	}
	str.replace(start, from.length(), to);
	return true;
}

static float randomFloat() {
	return (float)rand() / (float)RAND_MAX;
}

static float randomFloat(float start, float end) {
	return randomFloat() * (end - start) + start;
}

static int randomInt(int start, int end) {
	return (int)randomFloat(start, end + 1);
}

static glm::vec3 randomVec3() {
	return glm::vec3(randomFloat(), randomFloat(), randomFloat());
}