#version 330 core

uniform sampler2D btn;
uniform int state;

in vec2 txcoords;

out vec4 frag_color;

void main() {
  if(state == 0) {
    frag_color = texture(btn, txcoords).rgba;
  } else if(state == 1) {
    vec4 color = vec4(texture(btn, txcoords));
    frag_color = vec4(color.rgb + .3, color.a);
  } else if(state == 2) {
    float brightness = length(txcoords - .5) / sqrt(2.) + .2;
    if(brightness < .3) brightness = .3;
    vec4 color = vec4(texture(btn, txcoords));
    frag_color = vec4(color.rgb + brightness, color.a);
  } else discard;
}
