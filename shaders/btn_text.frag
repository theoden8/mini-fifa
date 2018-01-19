#version 330

in vec2 txcoords;
out vec4 frag_color;

uniform sampler2D font_texture;
uniform vec3 textcolor;

void main(void) {
  float res = texture(font_texture, txcoords).r;
  if(res > .5) {
    frag_color = vec4(textcolor, 1.0);
  } else {
    frag_color = vec4(0.0, 0.0, 0.0, 0.0);
  }
}
