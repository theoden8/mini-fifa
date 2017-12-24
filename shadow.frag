#version 330 core

in vec2 pos_xy;
out vec4 frag_color;

void main(void) {
  float value = (length(pos_xy) / sqrt(2)) / 3;
  if(value > .3)discard;
  frag_color = vec4(.3, .3, .3, .3 - value);
}
