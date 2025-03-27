#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 vertColor;
out vec2 texCoord;

uniform vec2 pos;
uniform vec2 size;

void main() {
    vertColor = aColor;
    texCoord = aTexCoord;

    gl_Position = vec4(
            aPos.x * size.x + pos.x - (1 - size.x),
            aPos.y * size.y + pos.y - (1 - size.y),
            aPos.z,
            1.0f
        );
}
