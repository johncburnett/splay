#version 120

uniform sampler2DRect tex0;
uniform sampler2DRect tex1;
uniform float opacity;

void main() {
    gl_FragColor = vec4(1, 1, 1, opacity);
}
