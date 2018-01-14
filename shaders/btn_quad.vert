#version 330

uniform mat4 transform;

layout (location = 0) in vec2 vertex;

out vec2 txcoords;

void main() {
  txcoords = vertex;
  gl_Position = transform * vec4(vertex, 0.0, 1.0);
}
