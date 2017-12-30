#version 330 core

in vec2 vertex;
out vec2 pos_xy;

void main(void) {
  gl_Position = vec4(vertex, 0, 1);
  pos_xy = vertex;
}
