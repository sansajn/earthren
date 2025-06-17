#version 320 es
precision highp float;

// Only position attribute
layout(location = 0) in vec2 position;  // Positions like (-1,-1), (1,-1), (-1,1), (1,1)

// Output to fragment shader
out vec2 vTexCoord;

void main() {
	gl_Position = vec4(position, 0.0, 1.0);
	
	// Generate texture coordinates from position
	// Convert from [-1,1] range to [0,1] range
	vTexCoord = position * 0.5 + 0.5;
}
