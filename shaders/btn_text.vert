#version 330

layout (location = 0) in vec4 vertex;
out vec2 txcoords;

uniform mat4 transform;

void main(void) {
  gl_Position = transform * vec4(vertex.xy, 0.0, 1.0);
  txcoords = vertex.zw;
}
