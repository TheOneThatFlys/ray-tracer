#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>

class Shader
{
public:
	GLuint ID;
	Shader();
	Shader(const char* vertexFile, const char* fragFile);
	void SetDefine(std::string from, int to);
	void Create();
	void Activate();
	void Delete();
	void CompileErrors(GLuint shader, int type);
	static std::string LoadSourceFromPath(const char* path);
	void setInt(const char* name, int v);
	void setFloat(const char* name, float v);
private:
	std::string vertexCode;
	std::string fragCode;
};

enum ShaderType {
	VERTEX, PROGRAM, FRAGMENT
};

