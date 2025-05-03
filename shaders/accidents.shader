// Concentric rings
vec2 cUV = vUV.xy - 0.5;
float i;
FragColor = vec4(modf(length(cUV) * ((t / 100) % 10), i) , 0, 0, color.a * alpha);