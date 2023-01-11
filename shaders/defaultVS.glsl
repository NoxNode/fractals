#version 410

#define MAX_JOINTS_PER_MODEL 100
#define MAX_WEIGHTS 4

// in
layout(location=0) in vec4 a_position;
layout(location=1) in vec4 a_uvCoords;
layout(location=2) in vec4 a_normal;
layout(location=3) in vec4 a_tangent;
// layout(location=4) in vec4 a_bitangent;
layout(location=5) in vec4 a_jointIndices;
layout(location=6) in vec4 a_jointWeights;

// out
out vec4 v_position;
out vec4 v_uvCoords;
out vec4 v_normal;
// out vec4 v_tangent;
// out vec4 v_bitangent;
out mat3 TBN;
out vec4 v_positionFromLight;

// matrices
uniform mat4 u_modelMatrix;
uniform mat4 u_normalMatrix;
uniform mat4 u_mvpMatrix;
uniform mat4 u_mvpMatrixFromLight;
uniform mat4 u_jointTransforms[MAX_JOINTS_PER_MODEL];

// flags
uniform bool lighting_enabled;
uniform bool diffuse_lighting_enabled;
uniform bool specular_lighting_enabled;
uniform bool ambient_lighting_enabled;
uniform bool texture_mapping_enabled;
uniform bool normal_mapping_enabled;
uniform bool displacement_mapping_enabled;
uniform bool shadow_mapping_enabled;
uniform bool shadow_cube_mapping_enabled;
uniform bool skeletal_animations_enabled;

// mat4 inverse(mat4 m);

void main() {
	mat4 jointTransform = mat4(1.0);
	if(skeletal_animations_enabled) {
		jointTransform = mat4(0);
		for(int i = 0; i < MAX_WEIGHTS; i++) {
			if(a_jointWeights[i] <= 0.0f) continue;
			jointTransform += u_jointTransforms[int(a_jointIndices[i])] * a_jointWeights[i];
		}
	}
	// TODO: supposedly we can just transpose if we're only doing translation and rotation
		// according to https://community.khronos.org/t/new-challenge-inverse-transpose-matrix-under-glsl-120/67627/3
	mat4 jointNormalTransform = transpose(inverse(jointTransform));

	gl_Position = u_mvpMatrix * jointTransform * a_position;
	v_position = u_modelMatrix * jointTransform * a_position;

	if(texture_mapping_enabled) {
		v_uvCoords = a_uvCoords;
	}
	if(lighting_enabled) {
		v_normal = u_normalMatrix * jointNormalTransform * a_normal;
	}
	if(normal_mapping_enabled || displacement_mapping_enabled) {
		// v_tangent = u_normalMatrix * a_tangent;
		// v_bitangent = u_normalMatrix * a_bitangent;
		vec3 N = normalize((u_normalMatrix * jointNormalTransform * vec4(a_normal.xyz, 0)).xyz);
		vec3 T = normalize((u_normalMatrix * jointNormalTransform * vec4(a_tangent.xyz, 0)).xyz);
		T = normalize(T - dot(T, N) * N);
	    vec3 B = cross(T, N);
		TBN = mat3(T, B, N);
	}
	if(shadow_mapping_enabled) {
		v_positionFromLight = u_mvpMatrixFromLight * jointTransform * a_position;
	}
}

// mat4 inverse(mat4 m) {
//   float
//       a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3],
//       a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3],
//       a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3],
//       a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3],

//       b00 = a00 * a11 - a01 * a10,
//       b01 = a00 * a12 - a02 * a10,
//       b02 = a00 * a13 - a03 * a10,
//       b03 = a01 * a12 - a02 * a11,
//       b04 = a01 * a13 - a03 * a11,
//       b05 = a02 * a13 - a03 * a12,
//       b06 = a20 * a31 - a21 * a30,
//       b07 = a20 * a32 - a22 * a30,
//       b08 = a20 * a33 - a23 * a30,
//       b09 = a21 * a32 - a22 * a31,
//       b10 = a21 * a33 - a23 * a31,
//       b11 = a22 * a33 - a23 * a32,

//       det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

//   return mat4(
//       a11 * b11 - a12 * b10 + a13 * b09,
//       a02 * b10 - a01 * b11 - a03 * b09,
//       a31 * b05 - a32 * b04 + a33 * b03,
//       a22 * b04 - a21 * b05 - a23 * b03,
//       a12 * b08 - a10 * b11 - a13 * b07,
//       a00 * b11 - a02 * b08 + a03 * b07,
//       a32 * b02 - a30 * b05 - a33 * b01,
//       a20 * b05 - a22 * b02 + a23 * b01,
//       a10 * b10 - a11 * b08 + a13 * b06,
//       a01 * b08 - a00 * b10 - a03 * b06,
//       a30 * b04 - a31 * b02 + a33 * b00,
//       a21 * b02 - a20 * b04 - a23 * b00,
//       a11 * b07 - a10 * b09 - a12 * b06,
//       a00 * b09 - a01 * b07 + a02 * b06,
//       a31 * b01 - a30 * b03 - a32 * b00,
//       a20 * b03 - a21 * b01 + a22 * b00) / det;
// }
