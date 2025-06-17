#version 320 es
precision highp float;

// Input from vertex shader
in vec2 vTexCoord;

// Input texture from previous pass
uniform sampler2D inputTexture;
uniform vec2 texelSize;

// Output
layout(location = 0) out vec4 resultValue;

void main() {
	// Calculate the four sampling points using the center coordinate from vertex shader
	vec2 texCoordNW = vTexCoord + vec2(-texelSize.x, -texelSize.y);
	vec2 texCoordNE = vTexCoord + vec2( texelSize.x, -texelSize.y);
	vec2 texCoordSW = vTexCoord + vec2(-texelSize.x,  texelSize.y);
	vec2 texCoordSE = vTexCoord + vec2( texelSize.x,  texelSize.y);
	
	// Sample texels
	vec4 value1 = texture(inputTexture, texCoordNW);
	vec4 value2 = texture(inputTexture, texCoordNE);
	vec4 value3 = texture(inputTexture, texCoordSW);
	vec4 value4 = texture(inputTexture, texCoordSE);
	
	// Perform reduction operation
	resultValue = max(max(max(value1, value2), value3), value4);
}
