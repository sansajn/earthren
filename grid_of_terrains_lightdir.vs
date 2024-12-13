#version 320 es

// Vertex shader proram to visualize light direction.

precision mediump usampler2D;

layout(location = 0) in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec3 d;  // direction output for geometry shader

uniform usampler2D heights;  // 16bit UI height texture [0, 65535]
uniform float elevation_scale;  // terrain elevation scale factor calculated from elevation pixel resolution (e.g. = 0.000107174)
uniform float height_scale;  // e.g. 1 for PNG files or 100 for TIFF (elevation) files

const vec3 light_direction = vec3(0.86, 0.14, 0.49);  // TODO: Is this in word coordinate system?

void main() {
	d = light_direction;  // pass light direction to a geometry shader

   // read h value from height map
	float h = float(texture(heights, position.xy).r) * elevation_scale * height_scale;

	gl_Position = vec4(position.xy, h, 1);  // position in a local coordinate system
}
