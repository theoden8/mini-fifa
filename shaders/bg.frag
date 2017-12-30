#version 330 core

in vec2 pos_xy;
out vec4 frag_color;

void main(void) {
  if(pos_xy.y < .2)discard;
  frag_color = vec4(0, sin(pos_xy.y - .2) * 2./3 * 1.5, sin(pos_xy.y - .2) * 1.5, 1);
}
