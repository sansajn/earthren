#version 320 es

// fragment shader for `height_sinxy_normals` sample

precision mediump float;

in vec3 color;
out vec4 frag_color;

void main() {
   frag_color = vec4(color, 1.0);
}
