#version 410

#define MAX_LIGHTS 4

in vec4 v_position;
in vec4 v_uvCoords;
in vec4 v_normal;
// in vec4 v_tangent;
// in vec4 v_bitangent;
in mat3 TBN;
in vec4 v_positionFromLight;

uniform vec3 u_cameraPos;

// flags
uniform bool lighting_enabled;
uniform bool diffuse_lighting_enabled;
uniform bool specular_lighting_enabled;
uniform bool ambient_lighting_enabled;
uniform bool texture_mapping_enabled;
uniform bool normal_mapping_enabled;
uniform bool displacement_mapping_enabled;
uniform bool shadow_mapping_enabled;
uniform bool pcf_enabled;
uniform bool shadow_cube_mapping_enabled;

// material
uniform sampler2D u_texture;
uniform sampler2D u_normalMap;
uniform sampler2D u_dispMap;
uniform sampler2D u_dispMap2;
uniform float u_dispMapScale;
uniform float u_dispMapBias;
uniform float u_shine;
uniform vec3 u_color;

// directional light
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform sampler2D u_shadowMap;

// point / spot lights
struct Light {
	vec3 pos;
    vec3 lookDir;
	vec3 color;

    // for spot lights
    float angle;

    // coefficients that make up the fade out function
    float constant;
    float linear;
    float quadratic;
	
	// for shadows
	float far_plane;
	//samplerCube shadowMap;
};
uniform Light u_lights[MAX_LIGHTS];
uniform int u_nLights;

vec2 CalcDisplacedUVCoords(mat3 TBN, sampler2D dispMap, vec2 uvCoords, vec3 viewVec, float dispMapScale, float dispMapBias);
vec3 CalcBumpedNormal(mat3 TBN, sampler2D normalMap, vec2 uvCoords);
vec3 CalcDiffuse(vec3 lightColor, vec3 materialColor, float nDotL);
vec3 CalcSpecular(vec3 lightColor, vec3 materialColor, vec3 lightDir, vec3 normal, vec3 viewVec, float shine);
float CalcShadow(vec4 posFromLight, sampler2D shadowMap);
//float CalcCubeShadow(vec3 posDiff, float distance, float far_plane, samplerCube shadowMap);

out vec4 fragColor;

void main() {
	vec3 currentColor = vec3(0.0, 0.0, 0.0);

	vec3 viewVec = normalize(u_cameraPos - v_position.xyz);
	vec2 uvCoords = v_uvCoords.xy;
	vec3 normal = normalize(v_normal.xyz);
	if(displacement_mapping_enabled) {
		uvCoords = CalcDisplacedUVCoords(TBN, u_dispMap, uvCoords, viewVec, u_dispMapScale, u_dispMapBias);
	}
	if(normal_mapping_enabled) {
		normal = CalcBumpedNormal(TBN, u_normalMap, uvCoords);
	}
	vec3 materialColor = u_color;
	if(texture_mapping_enabled) {
		materialColor *= texture(u_texture, uvCoords).xyz;
	}

	if(!lighting_enabled) {
		currentColor = materialColor;
	}
	else {
		// directional light	
		float visibility = 1.0;
		vec3 lightContribution = vec3(0.0, 0.0, 0.0);
		vec3 lightDir = -normalize(u_lightDir);
		float nDotL = dot(lightDir, normal);
		if(nDotL > 0.0) {
			if(diffuse_lighting_enabled) {
				lightContribution += CalcDiffuse(u_lightColor, materialColor, nDotL);
			}
			if(specular_lighting_enabled) {
				lightContribution += CalcSpecular(u_lightColor, materialColor, lightDir, normal, viewVec, u_shine);
			}
			if(shadow_mapping_enabled) {
				float shadow = CalcShadow(v_positionFromLight, u_shadowMap);
				visibility = 1.0 - shadow;
			}
		}
		currentColor += lightContribution * visibility;

		// point / spot lights
		for(int i = 0; i < u_nLights; i++) {
			vec3 posDiff = u_lights[i].pos - v_position.xyz;
			lightDir = normalize(posDiff);
			lightContribution = vec3(0.0, 0.0, 0.0);
			nDotL = dot(lightDir, normal);
			if(nDotL > 0.0) {
				float distance = length(posDiff);
				if(diffuse_lighting_enabled) {
					lightContribution += CalcDiffuse(u_lights[i].color, materialColor, nDotL);
				}
				if(specular_lighting_enabled) {
					lightContribution += CalcSpecular(u_lights[i].color, materialColor, lightDir, normal, viewVec, u_shine);
				}
				//if(shadow_cube_mapping_enabled) {
				//	float shadow = CalcCubeShadow(posDiff, distance, u_lights[i].far_plane, u_lights[i].shadowMap);
				//	visibility = 1.0 - shadow;
				//}
				if(u_lights[i].angle != 0) {
					// TODO: check if point is in angle
						// compare dot product of posDiff and u_lights[i].lookDir and get cos(theta)
				}
				float attenuation = 1 / (u_lights[i].constant + u_lights[i].linear * distance + u_lights[i].quadratic * distance * distance);
				lightContribution = attenuation * lightContribution * visibility;
			}
			currentColor += lightContribution;
		}
		
		if(ambient_lighting_enabled) {
			// TODO: maybe make a uniform for this 0.1
			currentColor += materialColor * 0.1;
		}
	} // end of else of if(!lighting_enabled)

	// store depth in alpha to merge with fractal objects
	float depth = length(u_cameraPos - v_position.xyz) / 10.0;

	fragColor = vec4(
		min(1.0, currentColor.r),
		min(1.0, currentColor.g),
		min(1.0, currentColor.b),
		depth
	);
}

vec2 CalcDisplacedUVCoords(mat3 TBN, sampler2D dispMap, vec2 uvCoords, vec3 viewVec, float dispMapScale, float dispMapBias) {
	float height = texture(u_dispMap, uvCoords).r * dispMapScale + dispMapBias;
	vec3 tbndViewVec = viewVec * TBN;
	return uvCoords + vec2(tbndViewVec.x * height, -tbndViewVec.y * height);
}

vec3 CalcBumpedNormal(mat3 TBN, sampler2D normalMap, vec2 uvCoords) {
    vec3 BumpMapNormal = 2.0 * texture(normalMap, uvCoords.xy).xyz - vec3(1.0, 1.0, 1.0);
    vec3 NewNormal = normalize(TBN * BumpMapNormal);
    return NewNormal;
}

vec3 CalcDiffuse(vec3 lightColor, vec3 materialColor, float nDotL) {
	return lightColor * materialColor * nDotL;
}

vec3 CalcSpecular(vec3 lightColor, vec3 materialColor, vec3 lightDir, vec3 normal, vec3 viewVec, float shine) {
	// halfway vector reflecting
	vec3 halfwayVec = (lightDir + viewVec) / length(lightDir + viewVec);
	return materialColor * lightColor * pow(max(0.0, dot(normal, halfwayVec)), shine);

	// perfect reflecting
	// vec3 reflectionVec = normalize(reflect(lightDir, normal));
	// return materialColor * lightColor * pow(max(0.0, dot(viewVec, reflectionVec)), shine);
}

float CalcShadow(vec4 posFromLight, sampler2D shadowMap) {
	vec3 shadowCoord = ((posFromLight.xyz)/posFromLight.w)*vec3(0.5) + vec3(0.5);

	// this makes things outside the light's 'camera' view not in shadow
	if(shadowCoord.z > 1.0)
		return 0.0;

	float curDepth = shadowCoord.z;
	float bias = 0.005;

	float shadow = 0.0;
	if(pcf_enabled) {
		// pcf (percentage-closer filtering)
		vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
		for(int x = -1; x <= 1; ++x) {
			for(int y = -1; y <= 1; ++y) {
				float pcfDepth = texture(shadowMap, shadowCoord.xy + vec2(x, y) * texelSize).r;
				if(curDepth - bias > pcfDepth) {
					shadow += 1.0;
				}
			}
		}
		// TODO: do 2 passes, first where we get the average depth
			// and a second where we weight the shadow contribution by the distance from the average depth
		shadow /= 9.0;
	}
	else {
		// hard shadows
		float closestDepth = texture(u_shadowMap, shadowCoord.xy).r;
		if(curDepth - bias > closestDepth) {
			shadow = 0.9;
		}
	}

	return shadow;
}

//float CalcCubeShadow(vec3 posDiff, float distance, float far_plane, samplerCube shadowMap) {
//	float shadow = 0.0;
//
//	// hard shadows
//	float closestDepth = texture(shadowMap, posDiff).r;
//	closestDepth *= far_plane;
//	if((distance - 0.005) > closestDepth) {
//		shadow = 0.9;
//	}
//
//	// TODO: pcf
//
//	return shadow;
//}
