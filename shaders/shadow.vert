#version 330 core

uniform mat4 transform;

layout (location = 0) in vec2 vertex;

out vec2 pos_xy;

void main(void) {
  gl_Position = transform * vec4(vertex, 0, 1);
  pos_xy = vertex;
}
