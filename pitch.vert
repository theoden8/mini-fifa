#version 400

uniform mat4 transform;

layout (location = 0) in vec3 triangle;

out vec2 txcoords;

void main(void) {
  gl_Position = transform * vec4(triangle, 1);
  txcoords = (1 + triangle.xy) / 2;
}
