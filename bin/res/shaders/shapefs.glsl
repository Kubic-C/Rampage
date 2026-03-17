#version 450

layout(location = 0) in vec3 v_color;       // match vertex shader output location
layout(location = 0) out vec4 outColor;    // output must also have a location

void main() {
    outColor = vec4(v_color, 1.0);
}