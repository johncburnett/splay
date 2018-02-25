#version 330

uniform float opacity;

out vec4 fragColor;

void main (void) {
    fragColor = vec4(1.0, 1.0, 1.0, opacity);
}
