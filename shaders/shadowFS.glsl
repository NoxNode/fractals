#version 410

out vec4 fragColor;

void main() {
	fragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 0.0); // Write the z-value in R
}
