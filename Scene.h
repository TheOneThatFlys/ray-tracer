#pragma once

#include "RenderQuad.h"
#include "Shader.h"
#include "BuffersStructs.h"
#include "Utils.h"
#include "Camera.h"

#include <vector>
#include <GLFW/glfw3.h>

class Scene
{
public:
	Camera camera;
	Scene() {};
	Scene(int width, int height, int samples = 8, int depth = 8);
	void Delete();
	void CalculateViewport();
	void ResizeCallback(int width, int height);
	void Render();
	void RenderTextureInit();
	void RenderTexture(int startY, int endY);
	void TextureToScreen();
	void AddSphere(SpheresBuffer s, MaterialBuffer m);
	void CalculateBVHs();
	GLuint getFrameBuffer() { return framebuffer; };
	void CreateBalls();

private:
	Shader shader;
	Shader textShader;
	RenderQuad quad;
	GLuint framebuffer;
	GLuint renderedTexture;
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	GLuint texID;

	glm::uvec2 imageSize;
	glm::vec3 cameraLookAt;

	glm::vec3 vup = glm::vec3(0, 1, 0);

	CameraBuffer cameraBuf;
	GLuint cameraUBO;
	std::vector<SpheresBuffer> spheres;
	GLuint spheresUBO;
	std::vector<BVHBuffer> bvhs;
	GLuint bvhUBO;

	void createUniformBuffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const;
	void updateBuffer(GLuint ubo, size_t size, void* data);
};

