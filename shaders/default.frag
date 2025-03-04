#version 460 core

out vec4 FragColor;

in vec3 vertColor;
in vec2 texCoord;

uniform sampler2D tex0;

uniform float time;
uniform float delta;
uniform vec2 resolution;

void main() {
    // FragColor = texture(tex0, texCoord) * vec4(vertColor, 1.0);
    FragColor = texture(tex0, texCoord) * vec4(gl_FragCoord.xy / resolution, 0.0f, 1.0f);
}
