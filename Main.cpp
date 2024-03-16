#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "RenderQuad.h"
#include "Scene.h"
#include "Shader.h"

// TIMES: ---------
// v?.1 - spheres mem - 2522
// v2.0 - empty bvh -> gpu - 2630
// v2.1 bvh aabbs 2137
// v2.2 no loop aabb detection - 1900

// 1920x1080, 256 samples, 32 depth, 256 steps
// 33.5s

// MODE = 0 for interactable camera (WASD, SPACE, SHIFT, MOUSE) [lower samples & depth]
// MODE = 1 for render single image [higher quality]
#define MODE 1

class Window {
public:
	Window(int width, int height) {
		window = glfwCreateWindow(width, height, "Ray Tracing!", NULL, NULL);

		if (window == NULL) {
			std::cout << "Failed to create GLFW window\n";
			glfwTerminate();
		}
		glfwMakeContextCurrent(window);

		glfwSetErrorCallback(glErrorCallback);

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, onResize);
		glfwSetCursorPosCallback(window, onMouseMove);
		glfwSetScrollCallback(window, onMouseScroll);
		glfwSetKeyCallback(window, onKey);

		glm::uvec2 screenSize = getScreenSize();
		glfwSetWindowPos(window, screenSize.x / 2 - width / 2, screenSize.y / 2 - height / 2);

		gladLoadGL();
		glViewport(0, 0, width, height);

		Scene scene(width, height, 8, 8);
		scene_p = &scene;

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		while (!glfwWindowShouldClose(window)) {
			
			if (lastTime == 0.0f) {
				deltaTime = 1.0f / 60.0f;
				lastTime = static_cast<float>(glfwGetTime());
			}
			else {
				float nowTime = static_cast<float>(glfwGetTime());
				deltaTime = nowTime - lastTime;
				lastTime = nowTime;
			}

			processKeyInput();
			scene_p->Render();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	~Window() {
		scene_p->Delete();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	glm::uvec2 getScreenSize() {
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		return glm::uvec2(mode->width, mode->height);
	}

	void onResize(int width, int height) {
		scene_p->ResizeCallback(width, height);
	}

	void onMouseMove(float x, float y) {
		if (!focused) return;
		if (firstMouse) {
			lastx = x;
			lasty = y;
			firstMouse = false;
		}

		float dx = x - lastx;
		float dy = y - lasty;

		lastx = x;
		lasty = y;

		scene_p->camera.Rotate(dx, dy, deltaTime);
	}

	void onMouseScroll(float dy) {
		if (!focused) return;
		scene_p->camera.Zoom(dy, deltaTime);
	}

	void onKeyPress(int key) {
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, true);
		}
		else if (key == GLFW_KEY_T) {
			toggleFocus();
		}
		else if (key == GLFW_KEY_P) {
			std::cout << "----CAMERA INFO----" << "\n"
				<< "POSITION: " << toString(scene_p->camera.position) << "\n"
				<< "YAW: " << scene_p->camera.yaw << "\n"
				<< "PITCH: " << scene_p->camera.pitch << "\n"
				<< "FOV: " << scene_p->camera.fov << "\n";
		}
	}

private:
	GLFWwindow* window;
	Scene* scene_p;

	float lastTime, deltaTime = 0.0f;
	float lastx, lasty = 0.0f;
	bool firstMouse = true;
	bool focused = true;

	static void onResize(GLFWwindow* win, int width, int height) {
		Window* obj = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
		obj->onResize(width, height);
	}

	static void onMouseMove(GLFWwindow* win, double x, double y) {
		Window* obj = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
		obj->onMouseMove((float)x, (float)y);
	}

	static void onMouseScroll(GLFWwindow* win, double dx, double dy) {
		Window* obj = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
		obj->onMouseScroll((float)dy);
	}
	
	static void onKey(GLFWwindow* win, int key, int scancode, int action, int mods) {
		Window* obj = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
		if (action == GLFW_PRESS) {
			obj->onKeyPress(key);
		}
	}

	void processKeyInput() {

		if (!focused) return;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			scene_p->camera.Move(FOWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			scene_p->camera.Move(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			scene_p->camera.Move(UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			scene_p->camera.Move(DOWN, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			scene_p->camera.Move(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			scene_p->camera.Move(RIGHT, deltaTime);
	}

	void toggleFocus() {
		if (focused) {
			focused = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			focused = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPos(window, lastx, lasty);
		}
	}
};

class ImageRenderer {
public:
	ImageRenderer(int width, int height, int samples, int depth, int steps = 1) {
		auto start = std::chrono::high_resolution_clock::now();

		GLFWwindow* window = glfwCreateWindow(width, height, "Rendering...", NULL, NULL);
		glfwMakeContextCurrent(window);
		gladLoadGL();
		glViewport(0, 0, width, height);
		Scene scene(width, height, samples, depth);
		
		int step = height / steps;

		scene.RenderTextureInit();
		for (int i = 0; i < height / step; i++) {
			scene.RenderTexture(i * step, std::min((i + 1) * step, height));
			scene.TextureToScreen();
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		
		unsigned char* pixels = (unsigned char*) malloc(width * height * 3);

		glBindFramebuffer(GL_FRAMEBUFFER, scene.getFrameBuffer());
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

		std::fstream output_image("output.ppm", std::ios::out | std::ios::trunc);
		output_image << "P3\n"
			<< width << " " << height << "\n"
			<< "255\n";

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int pos = (x + (height - y - 1) * width) * 3;
				output_image << (unsigned int) pixels[pos] << " " << (unsigned int) pixels[pos + 1] << " " << (unsigned int) pixels[pos + 2] << " ";
			}
			output_image << "\n";
		}

		free(pixels);
		output_image.close();

		auto end = std::chrono::high_resolution_clock::now();
		auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "Elapsed time: " << runtime.count() << "ms.\n";

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}

		scene.Delete();
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

int main() {
	srand(time(NULL));
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (MODE == 0) {
		Window window(1280, 720);
	}
	else {
		ImageRenderer r(1920, 1080, 256, 16, 256);
	}
	return 0;
}