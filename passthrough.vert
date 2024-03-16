#version 430 core

layout (location = 0) in vec2 aPos;

out vec2 UV;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    UV = (aPos + vec2(1, 1)) / 2;
}