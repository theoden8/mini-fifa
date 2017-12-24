#version 330 core

uniform mat4 transform;

layout (location = 0) in vec3 vpos;
layout (location = 1) in vec2 vtex;

out vec2 ftexcoords;

void main() {
  gl_Position = transform * vec4(vpos, 1.0);
  ftexcoords = vtex;
}
