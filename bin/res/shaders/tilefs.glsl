#version 450

layout(location = 0) in vec3 vTexCoords;

layout(set = 0, binding = 2) uniform sampler2DArray uSampler;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 frag = texture(uSampler, vTexCoords);

  if (frag.a <= 0.01)
    discard;

  fragColor = frag;
}