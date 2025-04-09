#version 460 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D screenTexture;

uniform vec2 res;

vec2 random(vec2 uv) {
    uv = vec2(dot(uv, vec2(127.1, 311.7)),
            dot(uv, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(uv) * 43758.5453123);
}

float to_vignette(vec2 uv) {
    vec2 new_uv = TexCoord;
    new_uv *= 1.0 - new_uv.yx;

    float vignette = new_uv.x * new_uv.y * 500.0;
    vignette = pow(vignette, 0.25);
    vignette = 1 - vignette;
    vignette = clamp(vignette, 0, 1);
    return vignette;
}

vec2 curve(vec2 uv) {
    vec2 new_uv = uv * 2 - 1;

    float curvature = 5;
    vec2 offset = new_uv / curvature;
    new_uv = new_uv + new_uv * offset * offset;

    return new_uv * 0.5 + 0.5;
}

float noise(vec2 uv) {
    vec2 uv_index = floor(uv);
    vec2 uv_fract = fract(uv);

    vec2 blur = smoothstep(0.0, 1.0, uv_fract);

    return mix(mix(dot(random(uv_index + vec2(0.0, 0.0)), uv_fract - vec2(0.0, 0.0)),
            dot(random(uv_index + vec2(1.0, 0.0)), uv_fract - vec2(1.0, 0.0)), blur.x),
        mix(dot(random(uv_index + vec2(0.0, 1.0)), uv_fract - vec2(0.0, 1.0)),
            dot(random(uv_index + vec2(1.0, 1.0)), uv_fract - vec2(1.0, 1.0)), blur.x), blur.y) * 0.5 + 0.5;
}

vec3 color_wave(vec2 uv) {
    float period = res.y * 2;
    float shift = 1;
    vec3 amp = vec3(0.1, 0.15, 0.1);
    return amp * vec3(cos(uv.y * period), sin(uv.y * period), cos(uv.y * period)) + shift;
}

void main() {
    vec2 uv = curve(TexCoord);
    float vignette = to_vignette(TexCoord);
    vec3 wave = color_wave(uv);

    vec4 tex = texture(screenTexture, uv);

    // if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) discard;
    // FragColor = vec4(wave * tex.rgb - vignette, tex.a);
    FragColor = texture(screenTexture, TexCoord);
}
