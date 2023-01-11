#include "../core/types.h"
#include "../gfx/render_obj.h"

/*
ray to triangle intersection
	find point on plane
	cross product edges with vector from vertex to point
	if the result of all those cross products all point in the direction of the triangle normal, its intersecting - sign is whether it hit back or front
ray to mesh intersection
	ray to triangle for all, take closest point (because might hit more than 1)
vector to plane intersection aka distance of point to plane
	https://youtu.be/GpsKrAipXm8?t=580
point in mesh test
	ray to center of mesh from point, if it hits back face first, it was inside
point in convex test
	point is under every plane
ray to convex cast + line segment with convex cast
	https://youtu.be/GpsKrAipXm8?t=904
intersection point of 3 planes
	v = -inverse(P)*b
	where P is mat3 of the normals of the planes
	b is the constant part of the planes (y offset I think)
	v is the intersection point
	if you can't inverse P some of the planes are parallel and there's no answer

TODO: try to implement quickhull (expanding volume)
	try to modify to detect if we've had any edges go outside the underlying geometry (ray cast along that edge, if it hits another triangle it must go outside)
		if so, ignore that edge and try to stay tight to the underlying
		if all next points are making an edge go outside the underlying, return what you have and start with one of those next points
	see if this can do nice convex decomposition

Notes for paper: https://matthias-research.github.io/pages/publications/realtimeCoursenotes.pdf
	another way of collision detection
		tetrahedralize the mesh
		split the bounding box into equal sized box partitions proportional in size to the average tetrahedron
		store a list of pointers to the tetrahedrons that are intersecting with that box partition in an array
		given a point to test collision with
			find the box partition it's in and test collision with the tetrahedrons that intersect with that box partition (pre-stored)
	I guess tetrahedralizing is faster than convex decomposition
	So you do this way for collision with soft bodies because you need to recompute the tetrahedralization or convex decomposition whenver the soft body deforms
	1st method (spring network)
		apply the following spring force to all vertices (can make springs between certain vertices or just make all vertices connected to all other)
		unified spring force = spring force + damping force
		spring force on i in a spring between point i and j = k * ij_hat * (len(ij) - spring_len) where k is the spring constant and ij_hat is the normalized vector from i to j
		damping force on i in spring between point i and j = kd * dot(vj - vi, ij_hat) where kd is the damping constant and vj and vi are the velocities of the points
		spring and damping foce on j is just negative the force on i
	2nd method (finite element)
		Hooke's Law (force per area applied to beam is proportional to the relative elongation)
		f/A=E*deltaL/l aka strain = E*stress aka applying strain results in stress
		
		strain depends on displacement from rest pose / material coordinate
		strain = transpose(deltaP)*deltaP - I
		where P is the transformation from rest pose to current position u
		another way of writing the equation is
		strain = deltaU + tranpose(deltaU) + transpose(deltaU) * deltaU
		stress - just look at page 25/26

		there's a bit more on this about rotations and stuff like that
Nd physics: https://www.youtube.com/watch?v=JpxZQxXxMWY&t=1140s



*/

// GJK algo: https://www.youtube.com/watch?v=MDusDn8oTSE
// if the convex polygon made up of subtracting each vertex to every vertex in the other object includes the origin, they're colliding

/*
running through an example of how CorrectPosition works
we have a penetration vector by doing penetration * normal,
we want to move A back proportional to its mass relative to B and vise versa for B
so lets say A->mass = 5, B->mass = 10
A->inv_mass / (A->inv_mass + B->inv_mass)
= 1/5 / (1/5 + 1/10)
= 1/5 / (2/10 + 1/10)
= 1/5 / (3/10)
= 1/5 * 10/3
= 2/3
and we want to move A by this amount because A has the smaller mass
similarly, B will be moved by 1 / 3 because it has a larger mass

we apply this same kind of scaling by mass concept to ResolveRigidCollision
*/

void CorrectPosition(RenderObj& a, RenderObj& b, const vec3& mtv) {
	v3 correction = mtv / (a.invMass + b.invMass);
	a.transform.pos -= a.invMass * correction;
	b.transform.pos += b.invMass * correction;
}

void ResolveRigidCollision(RenderObj& a, RenderObj& b, const vec3& mtv) {
	CorrectPosition(a, b, mtv);

	vec3 relativeVelocity = b.velocity - a.velocity;

	vec3 collisionNormal = normalize(mtv);

	r32 velAlongNormal = dot(relativeVelocity, collisionNormal);
	// make sure they're going toward each other
	if(velAlongNormal > 0)
		return;
	
	r32 restitution = std::min(a.material->restitution, b.material->restitution);

	// to perfectly reflect the velocity, reflection will be -2
	// so if we want to control how much we reflect, we need reflection to be between -1 and -2, based on the restitution aka bounciness
	// for why this is see https://hero.handmade.network/episode/code/day044#2062
	r32 reflection = -(1 + restitution);
	v3 projectedVel = velAlongNormal * collisionNormal;
	v3 impulse = projectedVel * reflection / (a.invMass + b.invMass);
	a.velocity -= impulse;
	b.velocity += impulse;
}



vec3 FindFurthestPoint(RenderObj& renderObj, Vertex* vertices, const vec3& dir) {
	vec3 maxPoint;
	r32 maxDist = -FLT_MAX;

	u32 verticesOffset = renderObj.model->verticesOffset;
	u32 numVertices = renderObj.model->numVertices;
	for(u32 i = verticesOffset; i < verticesOffset + numVertices; i++) {
		vec3 vertexPos = vec3(renderObj.transform.matrix * vec4(vertices[i].pos, 1.0f));
		
		r32 dist = dot(vertexPos, dir);
		if(dist > maxDist) {
			maxDist = dist;
			maxPoint = vertexPos;
		}
	}

	return maxPoint;
}

vec3 FindSupportVec(RenderObj& renderObj1, RenderObj& renderObj2, Vertex* vertices, const vec3& dir) {
	return FindFurthestPoint(renderObj1, vertices, dir) - FindFurthestPoint(renderObj2, vertices, -dir);
}

bool SameDirection(const vec3& direction, const vec3& ao) {
	return dot(direction, ao) > 0;
}

#define GJK_MAX_NUM_ITERATIONS 64
bool customGJK(RenderObj& renderObj1, RenderObj& renderObj2, Vertex* vertices, vec3* tetrahedron) {
	// vec3 tetrahedron[4];

	// first point doesn't matter
	vec3 support = FindSupportVec(renderObj1, renderObj2, vertices, vec3(1, 1, 1));
	tetrahedron[0] = support;

	// next point is farthest point in the direction to the origin
	vec3 direction = -support;
	support = FindSupportVec(renderObj1, renderObj2, vertices, direction);
	tetrahedron[1] = support;

	// if this next point is not past the origin, there's no way we're colliding
	if(dot(support, direction) < 0) return false;

	// the next direction is the normal of the line we just made with the first 2 points (the normal that's in the direction of the origin)
	vec3 p1ToOrigin = -tetrahedron[0];
	vec3 p1ToP2 = tetrahedron[1] - tetrahedron[0];
	vec3 up = cross(p1ToOrigin, p1ToP2);
	vec3 normal = cross(p1ToP2, up);
	direction = normal;
	if(direction == vec3(0,0,0)){ //origin is on this line segment (according to https://github.com/kevinmoran/GJK/blob/master/GJK.h, why not just return true here then)
        //Apparently any normal search vector will do?
        direction = cross(tetrahedron[0] - tetrahedron[1], vec3(1,0,0)); //normal with x-axis
        if(direction==vec3(0,0,0)) direction = cross(tetrahedron[0] - tetrahedron[1], vec3(0,0,-1)); //normal with z-axis
		printf("apparently this line segment passes through the origin?: (%f, %f, %f), (%f, %f, %f)\n", tetrahedron[0].x, tetrahedron[0].y, tetrahedron[0].z, tetrahedron[1].x, tetrahedron[1].y, tetrahedron[1].z);
    }

	// find the next point in that direction
	support = FindSupportVec(renderObj1, renderObj2, vertices, direction);
	tetrahedron[2] = support;

	// if this next point is not past the origin, there's no way we're colliding
	if(dot(support, direction) < 0) return false;

	// the next direction is the normal of the triangle we just made with the first 3 points (the normal that's in the direction of the origin)
	vec3 p1ToP3 = tetrahedron[2] - tetrahedron[0];
	normal = cross(p1ToP3, p1ToP2);
	// if the - normal is more in the direction of the origin than the initial origin we calculated, flip it (TODO: see if this is ever true)
	if(dot(normal, p1ToOrigin) < dot(-normal, p1ToOrigin)) {
		normal = -normal;
	}
	direction = normal;

	for(int i = 0; i < GJK_MAX_NUM_ITERATIONS; i++) {
		// find the next point in that direction
		support = FindSupportVec(renderObj1, renderObj2, vertices, direction);
		tetrahedron[3] = support;

		// if this next point is not past the origin, there's no way we're colliding
		if(dot(support, direction) < 0) return false;

		// now we have built a tetrahedron, so we check if the origin is within this tetrahedron
		// if not, we make a new tetrahedron by throwing away the oldest point and adding a new point that is generated by
			// looking in the direction of the normal of one of the faces such that the normal is most in the direction of the origin compared to other faces' normals
		// then after making that new tetrahedron, we again check if the origin is within the tetrahedron and so on

		// find the normal of the 3 other triangles of the tetrahedron that is most toward the origin (the normal of the 4th triangle is guaranteed away from the origin)
		vec3 p1ToP4 = tetrahedron[3] - tetrahedron[0];
		vec3 normal1 = cross(p1ToP4, p1ToP2);
		vec3 normal2 = cross(p1ToP3, p1ToP4);
		vec3 p2ToP4 = tetrahedron[3] - tetrahedron[1];
		vec3 p4ToP3 = tetrahedron[2] - tetrahedron[3];
		vec3 normal3 = cross(p2ToP4, p4ToP3);

		r32 dot1 = dot(normal1, p1ToOrigin);
		r32 dot2 = dot(normal2, p1ToOrigin);
		vec3 p2ToOrigin = -tetrahedron[1];
		r32 dot3 = dot(normal3, p2ToOrigin);

		// if none of the normals are in the direction of the origin, the origin must be inside the tetrahedron
		if(dot1 < 0 && dot2 < 0 && dot3 < 0) {
			// TODO: find the 3 points on the minkowski difference that are closest to the origin
				// then find the line that is perpendicular to that triangle that goes through the origin
				// that is the direction of the penetration vector
				// the magnitude of it is how far along that line we need to go to get to the origin
			return true;
		}
		// otherwise find the one with the highest dot product and go in that direction for the new point
		if(dot1 > dot2 && dot1 > dot3) {
			direction = normal1;
		}
		else if(dot2 > dot3) {
			direction = normal2;
		}
		else {
			direction = normal3;
		}

		// shift down so we can re-choose the 4th point
		for(int i = 1; i < 4; i++) {
			tetrahedron[i - 1] = tetrahedron[i];
		}
		// update vecs
		p1ToP2 = tetrahedron[1] - tetrahedron[0];
		p1ToP3 = tetrahedron[2] - tetrahedron[0];
		p1ToOrigin = -tetrahedron[0];
	}
	return false;
}

// rayLen = 0.0f to make the ray inifinite length
bool rayToBox(v3 max, v3 min, v3 rayOrig, v3 rayDir, r32 rayLen = 0.0f) {
	// find the 1 to 3 faces that the sword could possible intersect with
	// find the intersection point of the ray on the plane of each of the 3 faces
	// if any of those intersection points are within the square (the face on that plane) it's intersecting
	// if none of those intersection points are within the squares, it's not intersecting
		// need to handle corners though (maybe add some slack to the edges of the square)
	v3 halfDist = (max - min) / 2.0f;
	v3 center = min + halfDist;

	// find the 3 faces the ray could intersect with
	r32 xFace = rayOrig.x > center.x + halfDist.x ? halfDist.x : rayOrig.x < center.x - halfDist.x ? -halfDist.x : 0.0f;
	r32 yFace = rayOrig.y > center.y + halfDist.y ? halfDist.y : rayOrig.y < center.y - halfDist.y ? -halfDist.y : 0.0f;
	r32 zFace = rayOrig.z > center.z + halfDist.z ? halfDist.z : rayOrig.z < center.z - halfDist.z ? -halfDist.z : 0.0f;

	// find intersection of ray with x face
	if(xFace != 0.0f) {
		// it's pointing parallel and is outside the x bounds
		if(rayDir.x == 0.0f)
			return false;
		// find the scalar to scale rayDir by to intersect with the xFace
		r32 t = ((center.x + xFace) - rayOrig.x) / rayDir.x;
		// in order to intersect with the xFace, we need to go backwards
		if(t < 0.0f)
			return false;
		// now we know we have a valid candidate for an intersection on the xFace
		// so check if the point is in the xFace
		v3 intersection = rayOrig + rayDir * t;
		if((rayLen == 0.0f || t <= rayLen) &&
		intersection.y <= center.y + halfDist.y && intersection.y >= center.y - halfDist.y &&
		intersection.z <= center.z + halfDist.z && intersection.z >= center.z - halfDist.z) {
			// we're intersecting with the xFace, so we're intersecting with the cube
			return true;
		}
	}
	// find intersection of ray with y face
	if(yFace != 0.0f) {
		// it's pointing parallel and is outside the y bounds
		if(rayDir.y == 0.0f)
			return false;
		// find the scalar to scale rayDir by to intersect with the yFace
		r32 t = ((center.y + yFace) - rayOrig.y) / rayDir.y;
		// in order to intersect with the yFace, we need to go backwards
		if(t < 0.0f)
			return false;
		// now we know we have a valid candidate for an intersection on the yFace
		// so check if the point is in the yFace
		v3 intersection = rayOrig + rayDir * t;
		if((rayLen == 0.0f || t <= rayLen) &&
		intersection.x <= center.x + halfDist.x && intersection.x >= center.x - halfDist.x &&
		intersection.z <= center.z + halfDist.z && intersection.z >= center.z - halfDist.z) {
			// we're intersecting with the yFace, so we're intersecting with the cube
			return true;
		}
	}
	// find intersection of ray with z face
	if(zFace != 0.0f) {
		// it's pointing parallel and is outside the z bounds
		if(rayDir.z == 0.0f)
			return false;
		// find the scalar to scale rayDir by to intersect with the zFace
		r32 t = ((center.z + zFace) - rayOrig.z) / rayDir.z;
		// in order to intersect with the zFace, we need to go backwards
		if(t < 0.0f)
			return false;
		// now we know we have a valid candidate for an intersection on the zFace
		// so check if the point is in the zFace
		v3 intersection = rayOrig + rayDir * t;
		if((rayLen == 0.0f || t <= rayLen) &&
		intersection.x <= center.x + halfDist.x && intersection.x >= center.x - halfDist.x &&
		intersection.y <= center.y + halfDist.y && intersection.y >= center.y - halfDist.y) {
			// we're intersecting with the zFace, so we're intersecting with the cube
			return true;
		}
	}

	// of all the 3 possible faces we could intersect with, we don't, so return false
	return false;
}

bool pointInTraingle(r32 x, r32 y, v2 p1, v2 p2, v2 p3) {
    v2 point = v2(x, y);
    v3 p1_p = v3(point - p1, 0.0f);
    v3 p2_p = v3(point - p2, 0.0f);

    v3 p1_2 = v3(p2 - p1, 0.0f);
    v3 p1_3 = v3(p3 - p1, 0.0f);

    v3 up = v3(0, 0, 1);

    v3 n1_2 = cross(p1_2, up);
    v3 n1_3 = cross(up, p1_3);

    r32 dir1_2 = dot(p1_p, n1_2);
    r32 dir1_3 = dot(p1_p, n1_3);

    if(dir1_2 > 0 || dir1_3 > 0)
        return false;

    // now we know the point is inside p1_2 and p1_3
    // just have to check p2_3 now
    v3 p2_3 = v3(p3 - p2, 0.0f);

    v3 n2_3 = cross(p2_3, up);

    r32 dir2_3 = dot(p2_p, n2_3);

    if(dir2_3 > 0)
        return false;

    // the point is inside the triangle

    return true;
}

// Given three colinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
bool onSegment(vec2 p, vec2 q, vec2 r)
{
    if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
        q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
       return true;
 
    return false;
}
 
// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
float segmentOrientation(vec2 p, vec2 q, vec2 r)
{
    // See https://www.geeksforgeeks.org/orientation-3-ordered-points/
    // for details of below formula.
    float val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);
 
    if (val == 0) return 0;  // colinear
 
    return (val > 0)? 1: 2; // clock or counterclock wise
}
 
// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool segmentIntersect(vec2 p1, vec2 q1, vec2 p2, vec2 q2)
{
    // Find the four orientations needed for general and
    // special cases
    float o1 = segmentOrientation(p1, q1, p2);
    float o2 = segmentOrientation(p1, q1, q2);
    float o3 = segmentOrientation(p2, q2, p1);
    float o4 = segmentOrientation(p2, q2, q1);
 
    // General case
    if (o1 != o2 && o3 != o4)
        return true;
 
    // Special Cases
    // p1, q1 and p2 are colinear and p2 lies on segment p1q1
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;
 
    // p1, q1 and q2 are colinear and q2 lies on segment p1q1
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;
 
    // p2, q2 and p1 are colinear and p1 lies on segment p2q2
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;
 
     // p2, q2 and q1 are colinear and q1 lies on segment p2q2
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;
 
    return false; // Doesn't fall in any of the above cases
}

// TODO: take in info about the fractal obj (lambda function for sdf mainly)
bool collisionFractalSphere(Vertex* vertices, IndexType* indices, RenderObj& obj1) {
	Model* model1 = obj1.model;

	int indicesOffset1 = model1->indicesOffset;
	int verticesOffset1 = model1->verticesOffset;
	
	r32 closestDist = 10000.0;
	int closestIndex = -1;
	for(int i = 0; i < model1->numVertices; i++) {
		vec3 vertex = vertices[indices[indicesOffset1 + i]].pos;
		r32 dist = length(vertex - vec3(0.0, 1.0, 0.0)) - 0.5;
		if(dist < closestDist) {
			closestDist = dist;
			closestIndex = i;
		}
		if(dist < 0) return true;
	}
	// have closest point
	// loop through all triangles this point belongs to
	// find the closest point on those triangles to the geometry
	// average those points
	// then just 3d march in all directions towards the geometry

	// for fractal to fractal collision, just find any point on the first geometry, then 3d march within that first geomoetry towards the second
		// only works for sdfs that describe one contiguous object

	return false;
}

// draw line from control point to vertex for each convex subset of the model
// check that line collision with each triangle of the other model
// do this for both
// if any lines intersect with any triangles, they're colliding
bool old_collision3d(Vertex* vertices, IndexType* indices, RenderObj& obj1, RenderObj& obj2) {
	Model* model1 = obj1.model;
	Model* model2 = obj2.model;

	// TODO: control points - call this a second time with the guy model as the first object
		// draw lines from the nearest control point to the vertex and use that in the below calculations
		// make another control point for each shoulder
		// could also try making a function to generate control points and store that info in the render obj
// 	-5.399997, -0.400000, 3.499999
// 3.999999, 0.500000, 3.699999
// -1.200000, 0.600000, 39.499969
// 23.300053, -0.500000, 35.100037
// -22.800051, -0.600000, 35.300034
// -0.500000, 2.500000, 75.399422

	int indicesOffset1 = model1->indicesOffset;
	int indicesOffset2 = model2->indicesOffset;

	int verticesOffset1 = model1->verticesOffset;
	int verticesOffset2 = model2->verticesOffset;

	// we want the center of model1
	vec3 center = vec3(0.0f);
	for(int i = 0; i < model1->numVertices; i++) {
		vec3 vertex = vec3(vertices[verticesOffset1 + i].pos.x,
			vertices[verticesOffset1 + i].pos.y,
			vertices[verticesOffset1 + i].pos.z);
		// transform vertex based on the model matrix
		// vertex = vec3(obj1.transform.matrix * vec4(vertex, 1.0f));
		center += vertex;
	}
	center /= model1->numVertices;

	// printf("center: %f, %f, %f\n", center.x, center.y, center.z);

	// we want lines to each of the vertices of model1 from the center
	for(int i = 0; i < model1->numIndices; i++) {
		vec3 vertex = vertices[indices[indicesOffset1 + i]].pos;
		// transform vertex based on the model matrix
		// vertex = vec3(obj1.transform.matrix * vec4(vertex, 1.0f));

		vec3 vectorToVertexFromCenter = vertex - center;
		// we want to iterate over each of those lines and each of the triangles of model2
		int numTriangles2 = model2->numIndices / 3;
		for(int j = 0; j < model2->numIndices; j += 3) {
			// for each of these pairs, do a line to triangle test
			vec3 vertex1 = vertices[indices[indicesOffset2 + j]].pos;
			// transform vertex based on the model matrix
			// vertex1 = vec3(obj2.transform.matrix * vec4(vertex1, 1.0f));

			vec3 vertex2 = vertices[indices[indicesOffset2 + j + 1]].pos;
			// transform vertex based on the model matrix
			// vertex2 = vec3(obj2.transform.matrix * vec4(vertex2, 1.0f));

			vec3 vertex3 = vertices[indices[indicesOffset2 + j + 2]].pos;
			// transform vertex based on the model matrix
			// vertex3 = vec3(obj2.transform.matrix * vec4(vertex3, 1.0f));

			vec3 normal = cross(vertex2 - vertex1, vertex3 - vertex1);

			// printf("vertex1: %f, %f, %f\n", vertex1.x, vertex1.y, vertex1.z);
			// printf("vertex2: %f, %f, %f\n", vertex2.x, vertex2.y, vertex2.z);
			// printf("vertex3: %f, %f, %f\n", vertex3.x, vertex3.y, vertex3.z);

			if(dot(vectorToVertexFromCenter,normal) != 0) {
				// TODD HOWARD
				vec3 diff = center - vertex1;
				r32 prod1 = dot(diff, normal);
				r32 prod2 = dot(vectorToVertexFromCenter, normal);
				r32 prod3 = prod1 / prod2;

				vec3 intersectionPoint = center - vectorToVertexFromCenter * prod3;

				if(length(intersectionPoint - center) > length(vectorToVertexFromCenter)) {
					continue;
				}
				
				// printf("intersectionPoint: %f, %f, %f\n", intersectionPoint.x, intersectionPoint.y, intersectionPoint.z);

				// point in triangle
				v3 p1_p = intersectionPoint - vertex1;
				v3 p2_p = intersectionPoint - vertex2;

				vec3 vec21 = vertex2 - vertex1;
				vec3 vec31 = vertex3 - vertex1;
				vec3 up = cross(vec21, vec31);
				vec3 normal21 = cross(vec21, up);
				if(dot(normal21, p1_p) > 0)
					continue;
				vec3 normal31 = cross(up, vec31);
				if(dot(normal31, p1_p) > 0)
					continue;
				vec3 vec23 = vertex3 - vertex2;
				vec3 normal23 = cross(vec23, up);
				if(dot(normal23, p2_p) > 0)
					continue;

				// if any collide, the objects are colliding
				return true;
			}
		}
	}
	return false;
}
