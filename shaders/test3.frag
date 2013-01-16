#version 150 core

uniform vec3 cameraPos;
uniform vec3 lightPos1;

in vec3 pass_Position;
in vec3 pass_Normal;

out vec4 out_Color;

const float pi = 3.14159265358979323846264;

void main(void) {
    vec3 lightDiff1 = normalize(lightPos1 - pass_Position);
    vec3 viewer = cameraPos - pass_Position;
    float d = length(viewer);
    viewer = normalize(viewer);
    vec3 reflect = 2 * dot(lightDiff1, pass_Normal) * pass_Normal - lightDiff1;
    float angleDiff = dot(pass_Normal, lightDiff1) * 0.9;
    float angleSpec = dot(reflect, viewer) * 0.3;
    float color = clamp((angleDiff + angleSpec) * pow(10, (-0.001 * d)), 0, 1);
    out_Color = vec4(color, color, color, 1.0);
}
