#version 460

layout (location = 0) in vec3 inVertexColor;
layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(inVertexColor, 1.0);
}
