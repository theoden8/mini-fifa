#version 400

uniform sampler2D grass;

in vec2 txcoords;

out vec4 frag_color;

void main(void) {
  /* frag_color = vec4(txcoords.x, txcoords.y, 0.0, 1.0); */
  frag_color = texture(grass, txcoords).rgba;
  /* frag_color = 0.1 + texture(grass, vec2(0.5, 0.5)).rgba; */
}
