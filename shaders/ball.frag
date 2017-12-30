#version 330 core

uniform vec3 color;
uniform sampler2D ballTx;

in vec2 ftexcoords;

out vec4 frag_color;

void main() {
  frag_color = texture(ballTx, ftexcoords);
  frag_color = vec4(frag_color.x*color.x, frag_color.y*color.y, frag_color.z*color.z, frag_color.w);
}
