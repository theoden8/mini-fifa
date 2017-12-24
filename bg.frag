#version 330 core

in vec2 pos_xy;
out vec4 frag_color;

void main(void) {
  if(pos_xy.y < 0) {
    frag_color = vec4(0, 0, 0, 1);
  } else {
    frag_color = vec4(0, pos_xy.y/2, pos_xy.y, 1);
  }
}
