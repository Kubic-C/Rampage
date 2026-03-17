#version 450

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_color;

// Uniform buffer object (must be in a block)
layout(set = 0, binding = 0) uniform VP {
    mat4 u_vp;
} vp;

void main() {
  gl_Position = vp.u_vp * vec4(a_pos, 1.0);
  v_color = a_color;
}