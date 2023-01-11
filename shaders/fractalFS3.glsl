#version 410

const int MAX_MARCHING_STEPS = 256;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.0001;

vec4 orb = vec4(0,0,0,1); 

float orbFractalSDF( vec3 p, float s )
{
	float scale = 1.0;

	orb = vec4(1000.0); 
	
	for( int i=0; i<8;i++ )
	{
		p = -1.0 + 2.0*fract(0.5*p+0.5);

		float r2 = dot(p,p);
		
        orb = min( orb, vec4(abs(p),r2) );
		
		float k = s/r2;
		p     *= k;
		scale *= k;
	}
	
	return 0.25*abs(p.y)/scale;
}



/**
 * Signed distance function for a cube centered at the origin
 * with width = height = length = 2.0
 */
float cubeSDF(vec3 samplePoint, vec3 cubeCenter, vec3 sideLengths) {
    // If d.x < 0, then -1 < p.x < 1, and same logic applies to p.y, p.z
    // So if all components of d are negative, then p is inside the unit cube
    vec3 d = abs(samplePoint - cubeCenter) - sideLengths;
    
    // Assuming p is inside the cube, how far is it from the surface?
    // Result will be negative or zero.
    float insideDistance = min(max(d.x, max(d.y, d.z)), 0.0);
    
    // Assuming p is outside the cube, how far is it from the surface?
    // Result will be positive or zero.
    float outsideDistance = length(max(d, 0.0));
    
    return insideDistance + outsideDistance;
}

/**
 * Signed distance function for a sphere centered at the origin with radius 1.0;
 */
float sphereSDF(vec3 samplePoint, vec3 sphereCenter, float radius) {
    return length(samplePoint - sphereCenter) - radius;
}

/** pass in the result of a sdf and this will round the edges of the geometry */
float opRound( float fb, float rad ) { return fb - rad; }
/** pass in the result of 2 sdfs and this will return the union of the geometry */
float opUnion( float d1, float d2 ) { return min(d1,d2); }
/** pass in the result of 2 sdfs and this will return the subtraction of the geometry */
float opSubtraction( float d1, float d2 ) { return max(-d1,d2); }
/** pass in the result of 2 sdfs and this will return the intersection of the geometry */
float opIntersection( float d1, float d2 ) { return max(d1,d2); }


float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }

float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

float opSmoothIntersection( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h); }

// symmetry funcs need more testing dont seem to work as intended
vec3 opSymX( in vec3 p ) {
    p.x = abs(p.x);
    return p;
}
vec3 opSymXZ( in vec3 p ) {
    p.xz = abs(p.xz);
    return p;
}

vec3 opRep( in vec3 p, in vec3 c ) {
    return mod(p, c)-0.5*c;
}

vec3 opTranslate(vec3 samplePoint, vec3 translation) {
    return samplePoint - translation;
}

float testSDF( vec3 samplePoint ) {
    float fb = sphereSDF(samplePoint, vec3(0.0, 1.0, 0.0), 0.5);
    // fb = opSmoothUnion(fb, opRound(cubeSDF(samplePoint, vec3(-0.5, 1.0, 0.0), vec3(0.5, 0.5, 0.5)), 0.1), 0.2);

    // order of ops matter - can't translate after repeating
    // fb = opUnion( fb, cubeSDF(opRep(samplePoint, vec3(5.0, 5.0, 5.0)), vec3(0.0, 0.0, 0.0), vec3(0.5, 0.5, 0.5)) );
    
    return fb;
    // return orbFractalSDF(samplePoint, 1.1);

}

/**
 * Signed distance function describing the scene.
 * 
 * Absolute value of the return value indicates the distance to the surface.
 * Sign indicates whether the point is inside or outside the surface,
 * negative indicating inside.
 */
float sceneSDF(vec3 samplePoint) {
    // return cubeSDF(samplePoint);
    // return orbFractalSDF(samplePoint, 1.1);
    return testSDF(samplePoint);
}

/**
 * Return the shortest distance from the eyepoint to the scene surface along
 * the marching direction. If no part of the surface is found between start and end,
 * return end.
 * 
 * eye: the eye point, acting as the origin of the ray
 * marchingDirection: the normalized direction to march in
 * start: the starting distance away from the eye
 * end: the max distance away from the ey to march before giving up
 */
float shortestDistanceToSurface(vec3 eye, vec3 marchingDirection, float start, float end) {
    float depth = start;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        float dist = sceneSDF(eye + depth * marchingDirection);
        
	    float precis = 0.001 * depth;
        if (dist < precis) {
			return depth;
        }
        depth += dist;
        if (depth >= end) {
            return -1.0;
        }
    }
    return -1.0;
}
            

/**
 * Return the normalized direction to march in from the eye point for a single pixel.
 * 
 * fieldOfView: vertical field of view in degrees
 * size: resolution of the output image
 * gl_FragCoord: the x,y coordinate of the pixel in the output image
 */
vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord) {
    vec2 xy = fragCoord * 2.0 - size;
    float z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}

/**
 * Using the gradient of the SDF, estimate the normal on the surface at point p.
 */
vec3 estimateNormal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

/**
 * Lighting contribution of a single point light source via Phong illumination.
 * 
 * The vec3 returned is the RGB color of the light's contribution.
 *
 * k_a: Ambient color
 * k_d: Diffuse color
 * k_s: Specular color
 * alpha: Shininess coefficient
 * p: position of point being lit
 * eye: the position of the camera
 * lightPos: the position of the light
 * lightIntensity: color/intensity of the light
 *
 * See https://en.wikipedia.org/wiki/Phong_reflection_model#Description
 */
vec3 phongContribForLight(vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye,
                          vec3 lightPos, vec3 lightIntensity) {
    vec3 N = estimateNormal(p);
    vec3 L = normalize(lightPos - p);
    vec3 V = normalize(eye - p);
    vec3 R = normalize(reflect(-L, N));
    
    float dotLN = dot(L, N);
    float dotRV = dot(R, V);
    
    if (dotLN < 0.0) {
        // Light not visible from this point on the surface
        return vec3(0.0, 0.0, 0.0);
    } 
    
    if (dotRV < 0.0) {
        // Light reflection in opposite direction as viewer, apply only diffuse
        // component
        return lightIntensity * (k_d * dotLN);
    }
    return lightIntensity * (k_d * dotLN + k_s * pow(dotRV, alpha));
}

/**
 * Lighting via Phong illumination.
 * 
 * The vec3 returned is the RGB color of that point after lighting is applied.
 * k_a: Ambient color
 * k_d: Diffuse color
 * k_s: Specular color
 * alpha: Shininess coefficient
 * p: position of point being lit
 * eye: the position of the camera
 *
 * See https://en.wikipedia.org/wiki/Phong_reflection_model#Description
 */
vec3 phongIllumination(vec3 k_a, vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye) {
    const vec3 ambientLight = 0.5 * vec3(1.0, 1.0, 1.0);
    vec3 color = ambientLight * k_a;
    
    vec3 light1Pos = vec3(4.0 * sin(0.0),
                          2.0,
                          4.0 * cos(0.0));
    vec3 light1Intensity = vec3(0.4, 0.4, 0.4);
    
    color += phongContribForLight(k_d, k_s, alpha, p, eye,
                                  light1Pos,
                                  light1Intensity);
    
    vec3 light2Pos = vec3(2.0 * sin(0.37 * 0.0),
                          2.0 * cos(0.37 * 0.0),
                          2.0);
    vec3 light2Intensity = vec3(0.4, 0.4, 0.4);
    
    color += phongContribForLight(k_d, k_s, alpha, p, eye,
                                  light2Pos,
                                  light2Intensity);    
    return color;
}

/**
 * Return a transform matrix that will transform a ray from view space
 * to world coordinates, given the eye point, the camera target, and an up vector.
 *
 * This assumes that the center of the camera is aligned with the negative z axis in
 * view space when calculating the ray marching direction. See rayDirection.
 */
mat4 viewMatrix(vec3 eye, vec3 center, vec3 up) {
    // Based on gluLookAt man page
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    return mat4(
        vec4(s, 0.0),
        vec4(u, 0.0),
        vec4(-f, 0.0),
        vec4(0.0, 0.0, 0.0, 1)
    );
}


uniform vec3 u_cameraPos;
uniform vec3 u_cameraDir;
// uniform vec2 iResolution;

uniform sampler2D u_renderMap;
uniform sampler2D u_depthMap;

uniform mat4 u_vpMatrix;

out vec4 fragColor;

void main()
{
    vec2 iResolution = vec2(1200, 800);

	vec3 viewDir = rayDirection(45.0, iResolution.xy, gl_FragCoord.xy);
    vec3 eye = u_cameraPos;
    
    mat4 viewToWorld = viewMatrix(eye, u_cameraPos + u_cameraDir, vec3(0.0, 1.0, 0.0));
    
    vec3 worldDir = (viewToWorld * vec4(viewDir, 0.0)).xyz;
    
    float dist = shortestDistanceToSurface(eye, worldDir, MIN_DIST, MAX_DIST);
    
    if (dist == -1.0) {
        vec4 renderMapColor = texture(u_renderMap, gl_FragCoord.xy / iResolution.xy);
        float depthMapDepth = renderMapColor.a * 10.0;
        if((depthMapDepth < dist && renderMapColor.a != 1.0) || (dist == -1.0 && renderMapColor.a != 1.0)) {
            fragColor = renderMapColor;
        }
        else {
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        }
		return;
    }
    
    // The closest point on the surface to the eyepoint along the view ray
    vec3 p = eye + dist * worldDir;
    
    vec3 K_a = vec3(0.2, 0.2, 0.2);
    vec3 K_d = vec3(0.7, 0.2, 0.2);
    vec3 K_s = vec3(1.0, 1.0, 1.0);
    float shininess = 10.0;
    
    vec3 color = phongIllumination(K_a, K_d, K_s, shininess, p, eye);
    // fragColor = vec4(color, 1.0);
    
    vec4 renderMapColor = texture(u_renderMap, gl_FragCoord.xy / iResolution.xy);
    float depthMapDepth = renderMapColor.a * 10.0;
    // float depthMapDepth = texture(u_depthMap, gl_FragCoord.xy / iResolution.xy).r;
    if((depthMapDepth < dist && renderMapColor.a != 1.0) || (dist == -1.0 && renderMapColor.a != 1.0)) {
        fragColor = renderMapColor;
    }
    else {
    	fragColor = vec4(color, 1.0);
    }

    // const float near = EPSILON;
    // const float far = MAX_DIST;
    // const float a = (far+near)/(far-near);
    // const float b = 2.0*far*near/(far-near);
    // float z = length((u_cameraPos + viewDir * dist) - u_cameraPos);
    // gl_FragDepth = a + b/z;

    // if(dist != -1.0) {
    //     vec3 intersectionPoint = u_cameraPos + viewDir * dist;
    //     vec4 projectedPoint = (u_vpMatrix * vec4(intersectionPoint, 1.0));
    //     float zc = projectedPoint.z;
    //     float wc = projectedPoint.w;
    //     gl_FragDepth = zc / wc;
    // }
    // else {
    //     // gl_FragDepth = MAX_DIST;
    // }
}
