#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in vec2 localPos;
layout(location = 3) in vec2 worldPos;
layout(location = 4) in uint layer;
layout(location = 5) in float localRot;
layout(location = 6) in float z;
layout(location = 7) in uint dimByte;
layout(location = 8) in float scaling;

layout(location = 0) out vec3 vTexCoords;

layout(set = 0, binding = 0) uniform UBO {
  mat4 uVP;
};

layout(set = 0, binding = 1) uniform UBO2 {
  float uTileSideLength;
};

vec2 rotate(vec2 vec, float angle) {
  float cs = cos(angle);
  float sn = sin(angle);

  return vec2(
    vec.x * cs - vec.y * sn,
    vec.x * sn + vec.y * cs
  );
}

void main() {
  vec2 dim = vec2(dimByte & 15, (dimByte & 240) >> 4);

  vec2 transformed =
      worldPos +
      rotate(pos * uTileSideLength * dim + localPos, localRot) * scaling;

  gl_Position = uVP * vec4(transformed, z, 1.0);
  vTexCoords = vec3(texCoords * dim, layer);
}