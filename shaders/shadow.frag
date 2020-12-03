#version 330 core

in vec2 pos_xy;
out vec4 frag_color;

void main(void) {
  float value = (length(pos_xy) / sqrt(2.)) / 2;
  if(value > .5)discard;
  frag_color = vec4(0, 0, 0, .5 - value);
}
