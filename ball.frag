#version 330 core

uniform sampler2D ballTx;

in vec2 ftexcoords;

out vec4 frag_color;

void main() {
  frag_color = texture(ballTx, ftexcoords);
}
