#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec2 aUV;
layout (location=2) in vec3 aNrm;

out vec2 vUV;
out vec3 vNrm;
out vec3 vWorldPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 wpos = uModel * vec4(aPos, 1.0);
    vWorldPos = wpos.xyz;
    vNrm = mat3(transpose(inverse(uModel))) * aNrm;
    vUV = aUV;
    gl_Position = uProj * uView * wpos;
}
