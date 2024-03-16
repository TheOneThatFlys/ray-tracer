#version 460 core

in vec2 UV;

out vec3 colour;

uniform sampler2D renderedTexture;

void main() {
	colour = texture(renderedTexture, UV).rgb;
}