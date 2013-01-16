#version 150 core

in vec3 pass_Normal;

out vec4 out_Color;

void main(void) {
    out_Color = vec4(pass_Normal, 1.0);
}
