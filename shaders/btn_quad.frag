#version 330

uniform sampler2D btnTx;

in vec2 txcoords;

out vec4 frag_color;

void main() {
  frag_color = texture(btnTx, txcoords).rgba;
}
