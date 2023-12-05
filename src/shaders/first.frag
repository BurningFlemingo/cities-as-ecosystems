#version 460

layout (location = 0) in vec3 inVertexColor;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D texSampler;

void main() {
	outColor = texture(texSampler, inTexCoord);
}
