#include "Shader.h"
#include "Utils.h"
#include <iostream>

Shader::Shader() {}

Shader::Shader(const char* vertexPath, const char* fragPath) {
	vertexCode = LoadSourceFromPath(vertexPath);
	fragCode = LoadSourceFromPath(fragPath);
}

void Shader::SetDefine(std::string from, int to) {
	bool success = replace(fragCode, from, std::to_string(to == 0 ? 1 : to));
	if (!success) {
		fprintf(stderr, "No define found: %s", from.c_str());
		exit(20);
	}
}

void Shader::Create() {
	const char* vertexSource = vertexCode.c_str();
	const char* fragSource = fragCode.c_str();


	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	Shader::CompileErrors(vertexShader, VERTEX);

	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fragSource, NULL);
	glCompileShader(fragShader);
	Shader::CompileErrors(vertexShader, FRAGMENT);

	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragShader);
	glLinkProgram(ID);
	Shader::CompileErrors(ID, PROGRAM);

	glDeleteShader(vertexShader);
	glDeleteShader(fragShader);
}

std::string Shader::LoadSourceFromPath(const char* path) {
	std::ifstream stream(path, std::ios::in);
	std::string contents;
	std::string line;
	while (std::getline(stream, line)) {
		contents += line + "\n";
	}
	stream.close();
	return contents;
}

void Shader::Activate() {
	glUseProgram(ID);
}

void Shader::Delete() {
	glDeleteProgram(ID);
}

void Shader::setFloat(const char* name, float v) {
	glUniform1f(glGetUniformLocation(ID, name), v);
}

void Shader::setInt(const char* name, int v) {
	glUniform1i(glGetUniformLocation(ID, name), v);
}

void Shader::CompileErrors(GLuint shader, int type) {
	GLint hasCompiled;

	char infoLog[1024];
	if (type != PROGRAM) {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER COMPILATION ERROR\n" << infoLog << "\n";
		}
	}
	else {
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER LINKING ERROR\n" << infoLog << "\n";
		}
	}
}