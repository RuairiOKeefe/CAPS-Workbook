#version 440

uniform sampler2D tex;
uniform vec3 windVector;

// Incoming texture coordinate

// Outgoing fragment colour
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec4 colour;

void main()
{
	//colour = texture(tex, tex_coord);
	
	vec3 result = cross(windVector, position);
	colour = vec4(result.x, result.y, result.z, 1.0f);
}