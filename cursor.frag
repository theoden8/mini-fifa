#version 330 core

uniform sampler2D cursorTx;

in vec2 txcoords;

out vec4 frag_color;

void main(void) {
  frag_color = texture(cursorTx, txcoords).rgba;
}
