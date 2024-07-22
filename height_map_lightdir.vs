#version 320 es

// Vertex shader proram to visualize light direction.

precision mediump usampler2D;

in vec3 position;  // expected to be in a range of [0,1]^2 square
out vec3 d;  // direction output for geometry shader

uniform usampler2D heights;  // 16bit UI height texture
uniform vec2 height_map_size;  // height map (width, height) in pixels
uniform float height_scale;  // e.g. 1 for PNG files or 100 for TIFF (elevation) files

const float PI = 3.14159265359;
const vec3 light_direction = vec3(0.86, 0.14, 0.49);  // TODO: Is this in word coordinate system?

void main() {
   vec2 st = position.xy;
	d = light_direction;  // pass direction to a geometry shader

   // read h value from height map
	float h = float(texture(heights, st).r)/65535.0 * height_scale;  // pixel values are normalized to 0..1

	gl_Position = vec4(position.xy, h, 1);  // position in a local coordinate system
}
