#version 150

in vec2 texcoord;
in vec3 clr;
out vec4 frag_color;

uniform sampler2D tex;

void main() {
    frag_color = texture (tex, texcoord); // * vec4(1.0, 1.0, 1.0, 1.0);
}
