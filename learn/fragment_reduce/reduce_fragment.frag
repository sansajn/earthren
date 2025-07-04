#version 320 es
precision highp float;

// Input texture from previous pass
uniform sampler2D inputTexture;
uniform vec2 texelSize;

// Output
layout(location = 0) out vec4 resultValue;

void main() {
	// Get integer coordinates of the output pixel
	ivec2 outCoord = ivec2(gl_FragCoord.xy);

	// Compute the top-left input texture texel for this 2x2 block. Note, input texture is 2Wx2H size of the output texture.
	vec2 baseCoord = (vec2(outCoord) * 2.0 + 0.5) * texelSize;

	// Sample the 2x2 block
	vec4 value1 = texture(inputTexture, baseCoord);
	vec4 value2 = texture(inputTexture, baseCoord + vec2(texelSize.x, 0.0));
	vec4 value3 = texture(inputTexture, baseCoord + vec2(0.0, texelSize.y));
	vec4 value4 = texture(inputTexture, baseCoord + texelSize);

	resultValue = max(max(value1, value2), max(value3, value4));
}
