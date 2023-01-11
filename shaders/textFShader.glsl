#version 410

in vec2 texCoords;
// varying vec4 color;
out vec4 fragColor;

uniform sampler2D text;
uniform vec3 u_color;

void main()
{    
    // vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, texCoords).r);
    vec2 flipped_coords = texCoords;
    flipped_coords.y = 1-flipped_coords.y;
    fragColor = vec4(u_color, texture(text, flipped_coords.xy).r);
}  