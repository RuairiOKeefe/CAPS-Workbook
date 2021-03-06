#version 440

// Incoming vertex colour
layout (location = 0) in vec4 in_colour;

// Outgoing pixel colour
layout (location = 0) out vec4 out_colour;

void main()
{
	// Simply set outgoing colour
	out_colour = in_colour;
	// Alpha value set to 1 to ensure pixel is visable.
	out_colour.a =1.0f;
	out_colour = vec4(1,0,0,1);
}