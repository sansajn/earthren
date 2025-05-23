#version 320 es

precision mediump float;

uniform vec3 color;
uniform vec3 light_dir;  //! Normalized light direction e.g. normalize(vec3(1,1,1)) in view space (so we can calculate vwith normals).

in vec3 n;
out vec4 fcolor;  // output fragment color

void main() {
	fcolor = vec4(max(dot(n, light_dir), 0.2) * color, 1);
}
