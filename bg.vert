#version 400

in vec3 triangle;

out vec2 pos_xy;

void main(void) {
  gl_Position = vec4(triangle, 1.0);
  pos_xy = triangle.xy;
}
