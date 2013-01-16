#version 150 core

uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;

in vec3 in_Position;
in vec3 in_Normal;

out vec3 pass_Normal;

void main(void) {
    gl_Position = projectionMatrix * modelMatrix * vec4(in_Position, 1.0);
    pass_Normal = in_Normal;
}
