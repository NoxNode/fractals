#version 410

in vec4 v_uvCoords;

uniform sampler2D u_texture;
uniform vec3 u_color;

out vec4 fragColor;

void main() {
	// vec3 currentColor = u_color;
	// vec3 materialColor = texture(u_texture, v_uvCoords.xy).xyz;
	// currentColor *= materialColor;

	// fragColor = vec4(
	// 	min(1.0, currentColor.r),
	// 	min(1.0, currentColor.g),
	// 	min(1.0, currentColor.b),
	// 	1.0
	// );
	fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
