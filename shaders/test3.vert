#version 150 core

uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;
uniform mat4 modelTranslate;
uniform mat4 modelRotate;

in vec3 in_Position;
in vec3 in_Normal;

out vec3 pass_Position;
out vec3 pass_Normal;

void main(void) {
    vec4 position = modelTranslate * modelRotate * vec4(in_Position, 1.0);
    gl_Position = projectionMatrix * cameraMatrix * position;
    pass_Position = position.xyz;
    pass_Normal = (modelRotate * vec4(in_Normal, 1.0)).xyz;
}
