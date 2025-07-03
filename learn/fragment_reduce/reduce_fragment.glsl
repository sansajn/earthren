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
	//resultValue = max(max(max(value1, value2), value3), value4);

	resultValue.x = max(value1.x, max(value2.x, max(value3.x, value4.x)));
	resultValue.y = max(value1.y, max(value2.y, max(value3.y, value4.y)));
	resultValue.z = max(value1.z, max(value2.z, max(value3.z, value4.z)));
	resultValue.w = max(value1.w, max(value2.w, max(value3.w, value4.w)));

	//resultValue = texture(inputTexture, vTexCoord);
}
