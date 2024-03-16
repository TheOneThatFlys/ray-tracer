#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class RenderQuad
{
public:
	RenderQuad();
	void Render();
	void Delete();
private:
	GLuint VBO;
	GLuint VAO;
};

