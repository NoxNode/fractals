#include "../core/memory.h"
#include "../gfx/model.h"

#define MAX_NUM_HULLS 50

// TODO: maybe just give this function pre-tranformed vertices and for rigged models give the model in a pose that makes the most concavities it can have
void findConvHulls(Memory& mem, Model& model, Vertex* allVertices, IndexType* allIndices) {
    // vertices of model without duplicate positions
    vec3* vertices = (vec3*) malloc(model.numVertices * sizeof(vec3));
    int numVertices = 0;
    // indices of model adjusted for removed duplicate vertex positions
    IndexType* indices = (IndexType*) malloc(model.numIndices * sizeof(IndexType));
    int numIndices = model.numIndices;

    // populate indices
    for(int i = 0; i < model.numIndices; i++) {
        indices[i] = allIndices[model.indicesOffset + i];
    }

    // filter out duplicates as we populate vertices (and update indices accordingly)
    int verticesIndex = 0;
    for(int i = 0; i < model.numVertices; i++) {
        vertices[verticesIndex] = allVertices[model.verticesOffset + i].pos;
        // check for duplicates
        bool foundDuplicate = false;
        for(int j = verticesIndex - 1; j >= 0; j--) {
            if(vertices[verticesIndex] == vertices[j]) {
                // found a duplicate, so decrement all indices that are greater than or equal to this index
                for(int k = 0; k < model.numIndices; k++) {
                    if(indices[k] >= verticesIndex) {
                        indices[k]--;
                    }
                }
                foundDuplicate = true;
                break;
            }
        }
        if(!foundDuplicate) {
            // we didn't find a duplicate, so commit this vertex and go to the next one
            verticesIndex++;
        }
    }
    numVertices = verticesIndex;

    // which vertices are already used for a hull
    bool* isVertexUsed = (bool*) malloc(numVertices * sizeof(bool));
    for(int i = 0; i < numVertices; i++) {
        isVertexUsed[i] = false;
    }
    // which vertices can be used for the current hull (we stop trying to add vertices when all unused vertices cannot be used)
    bool* canVertexBeUsed = (bool*) malloc(numVertices * sizeof(bool));
    for(int i = 0; i < numVertices; i++) {
        canVertexBeUsed[i] = true;
    }
    // array of faces of the convex hulls we be makin
    vec3 hullFaces[numIndices / 3][4]; // each face has 3 points and its normal
    int numCurHullFaces = 0;
    // the starting index of each hull in reference to the hullFaces array
    IndexType* hullStartIndices = (IndexType*) malloc(MAX_NUM_HULLS * sizeof(IndexType));
    int curHullIndex = 0;

    // make first hull face out of the first face of the geometry
    hullFaces[0][0] = vertices[indices[0]];
    hullFaces[0][1] = vertices[indices[1]];
    hullFaces[0][2] = vertices[indices[2]];
    hullFaces[0][3] = cross(hullFaces[0][1] - hullFaces[0][0], hullFaces[0][2] - hullFaces[0][0]);
    numCurHullFaces++;

    isVertexUsed[indices[0]] = true;
    isVertexUsed[indices[1]] = true;
    isVertexUsed[indices[2]] = true;

    bool atLeastOneVertexCanBeUsed = false;
    int i = 0;
    while(i == numIndices && !atLeastOneVertexCanBeUsed) {
        if(i == numIndices) {
            i = 0;
            atLeastOneVertexCanBeUsed = false;
        }
        // if we're already using this vertex or have determined it cannot be used, skip
        if(isVertexUsed[indices[i]] || !canVertexBeUsed[indices[i]]) {
            continue;
        }
        // so we're now considering if we can use this vertex
        // let's assume it can and add the necessary faces to incorporate it
        // then we'll do some checks to see if the new hull we've made is still convex and within the original geometry

        // add the necessary faces

        i++;
    }
    
    // need to do 3 checks whenever we add a new point to the current volume
        // 1) are all the points coplanar?
            // if so, are all points contained within the geometry?
                // (maybe could reuse the step 3 code, but might have to do special stuff for coplanar points)
                // if so, just continue to the next point
            // if not all points are contained, remove the most recently added point
                // (TODO: handle only 3 points now - just add the point after the one just removed I guess)
        // 2) check if the current volume is concave
            // loop through all triangles of the volume
                // loop through all vertices of the volume that don't make up that triangle
                    // test to see which side the vertex is on of the plane made up of the triangle
                // if not all vertices are on the same side of the plane, it's concave
            // if concave, remove the most recently added point, mark it as unused, add the point after it, and continue
        // 3) check if the volume is contained within the given geometry
            // an alternative to the below is just when adding points, always add one such that one of points in the current volume is connected to the new point in the given geometry
                // this prevents making convex hulls for discontinuous geometry though, but I don't think we'd ever discontinuous geometry classified as the same model
            // loop through each edge (with points A and B)
                // loop through all triangles point B is a part of (except ones that A is also a part of)
                    // cast a ray from A to the center of that triangle
                    // if the ray hits a front face, then the edge is not contained within the geometry
                // do the above loop but use B as the reference point (this may not be necessary?)
                // if no rays hit a front face, then the edge is confirmed to be within / at the edge of the geometry
        // 4) does this new point pass through a hull we've already made (maybe don't need this if we're ok with intersecting hulls?)
            // can maybe just do gjk here
    // loop through each face and make sure all faces are pointing away from the center of the current volume
        // if not, fix them so that they are
    // after looping through all vertices that we could have added to the first convex hull, check if not all points are used
    // if not, make note of the curHull's startIndex, increment curHull, and take the first unused point as the next starting point and repeat above steps


    // a really slow way of making sure we have minimal number of separate hulls is:
        // do this algorithm starting with a different starting vertex each time we start a new hull
        // then just choose which ever combination of starting vertices gives the least number of hulls
    // this would be slow but I think it would guarantee us the least number of hulls
        // because we connect all points we can to the starting point
        // so if we do this for all possible starting points I think it will guarantee us the least number of hulls


    // free(hullStartIndices); // maybe dont need to free this - if so alloc with Alloc(mem, ...)
    // free(usedVertices);
    free(indices);
    free(vertices);
}
