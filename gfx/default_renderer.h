#pragma once
#include "default_shader.h"
#include "render_obj.h"
#include "material.h"
#include "light.h"
#include "gl_buffers.h"

struct DefaultRenderer {
	GLuint vao;
	// GLuint shadowVao;

    DefaultShader shader;
    GLuint shadowShader;
	FBO fbo;
    bool drawToFBO;
};

void InitDefaultRenderer(DefaultRenderer& renderer, VBO& vbo, IBO& ibo, bool drawToFBO, u32 fboWidth, u32 fboHeight) {
	InitDefaultShader(renderer.shader);
	InitShader(renderer.shadowShader, "shaders/shadowVS.glsl", "shaders/shadowFS.glsl");
    renderer.drawToFBO = drawToFBO;
    InitFBO(renderer.fbo, fboWidth, fboHeight);

	// create vao for default shader
	glUseProgram(renderer.shader.program);
	glGenVertexArrays(1, &renderer.vao);
	glBindVertexArray(renderer.vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	
	// init attributes for default shader
	GLint posLoc = glGetAttribLocation(renderer.shader.program, "a_position");
	GLint uvCoordsLoc = glGetAttribLocation(renderer.shader.program, "a_uvCoords");
	GLint noramlLoc = glGetAttribLocation(renderer.shader.program, "a_normal");
	GLint tangentLoc = glGetAttribLocation(renderer.shader.program, "a_tangent");
	// GLint bitangentLoc = glGetAttribLocation(renderer.shader.program, "a_bitangent");
	GLint jointIndicesLoc = glGetAttribLocation(renderer.shader.program, "a_jointIndices");
	GLint jointWeightsLoc = glGetAttribLocation(renderer.shader.program, "a_jointWeights");

	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	glVertexAttribPointer(uvCoordsLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, uvCoords));
	glVertexAttribPointer(noramlLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, normal));
	glVertexAttribPointer(tangentLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, tangent));
	// glVertexAttribPointer(bitangentLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, bitangent));
	glVertexAttribPointer(jointIndicesLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointIndices));
	glVertexAttribPointer(jointWeightsLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointWeights));

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(uvCoordsLoc);
	glEnableVertexAttribArray(noramlLoc);
	glEnableVertexAttribArray(tangentLoc);
	// glEnableVertexAttribArray(bitangentLoc);
	glEnableVertexAttribArray(jointIndicesLoc);
	glEnableVertexAttribArray(jointWeightsLoc);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.id);

	// // create vao for shadow shader
	// glUseProgram(renderer.shadowShader);
	// glGenVertexArrays(1, &renderer.shadowVao);
	// glBindVertexArray(renderer.shadowVao);
	// glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	
	// // init attributes for shadow shader
	// GLint shadowPosLoc = glGetAttribLocation(renderer.shadowShader, "a_position");
	// GLint shadowJointIndicesLoc = glGetAttribLocation(renderer.shadowShader, "a_jointIndices");
	// GLint shadowJointWeightsLoc = glGetAttribLocation(renderer.shadowShader, "a_jointWeights");

	// glVertexAttribPointer(shadowPosLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	// glVertexAttribPointer(shadowJointIndicesLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointIndices));
	// glVertexAttribPointer(shadowJointWeightsLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointWeights));

	// glEnableVertexAttribArray(shadowPosLoc);
	// glEnableVertexAttribArray(shadowJointIndicesLoc);
	// glEnableVertexAttribArray(shadowJointWeightsLoc);
	
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.id);
}

void DeinitDefaultRenderer(DefaultRenderer& renderer) {
	glDeleteVertexArrays(1, &renderer.vao);
	// glDeleteVertexArrays(1, &renderer.shadowVao);

	DeinitShader(renderer.shader.program);
	DeinitShader(renderer.shadowShader);
    DeinitTexture(renderer.fbo.texture);
}

void ShadowRender(DefaultRenderer& renderer, FBO& shadowMap, Camera& cameraForShadows, RenderObj* renderObjs, u32 nRenderObjs, JointBuffers& jointBuffers, u32 windowWidth, u32 windowHeight) {
	glViewport(0, 0, shadowMap.width, shadowMap.height);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMap.id);
	glClear(GL_DEPTH_BUFFER_BIT);

	// https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping Peter Panning - for solid objects, use the face closest to the floor
	glCullFace(GL_FRONT);

	// TODO: put these locations in a ShadowShader class
	GLuint u_mvpMatrixShadowPos = glGetUniformLocation(renderer.shadowShader, "u_mvpMatrix");
	GLuint skeletal_animations_enabledLoc = glGetUniformLocation(renderer.shadowShader, "skeletal_animations_enabled");
	GLuint jointTransformsLoc = glGetUniformLocation(renderer.shadowShader, "u_jointTransforms");
	for(u32 i = 0; i < nRenderObjs; i++) {
		RenderObj& obj = renderObjs[i];
		mat4 mvpMatrix = cameraForShadows.vpMatrix * obj.transform.matrix;
		glUniformMatrix4fv(u_mvpMatrixShadowPos, 1, GL_FALSE, &mvpMatrix[0][0]);

		if(obj.model->numJoints > 0) {
			CalcJointTransforms(obj, jointBuffers);
			glUniformMatrix4fv(jointTransformsLoc, MAX_JOINTS_PER_MODEL, GL_FALSE, &jointBuffers.jointTransforms[0][0][0]);
    		glUniform1i(skeletal_animations_enabledLoc, 1);
		}
		else {
    		glUniform1i(skeletal_animations_enabledLoc, 0);
		}

		glDrawElements(GL_TRIANGLES, renderObjs[i].model->numIndices, GL_UNSIGNED_INT, (void*)(renderObjs[i].model->indicesOffset * sizeof(IndexType)));
	}

	glCullFace(GL_BACK);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, windowWidth, windowHeight);
}

void BindMaterial(DefaultShader& shader, Material& material) {
	if(material.texture != NULL) {
		BindTexture(shader.u_texture, material.texture->texture, 0);
	}
	if(material.normalMap != NULL) {
		BindTexture(shader.u_normalMap, material.normalMap->texture, 1);
	}
	if(material.dispMap != NULL) {
		BindTexture(shader.u_dispMap, material.dispMap->texture, 2);
	}
	glUniform3fv(shader.u_color, 1, &material.color[0]);
	glUniform1f(shader.u_dispMapScale, material.dispMapScale);
	glUniform1f(shader.u_dispMapBias, material.dispMapBias);
}

void DefaultRender(DefaultRenderer& renderer, Camera& camera, RenderObj* renderObjs, u32 nRenderObjs, DirLight dirLight, Light* lights, u32 nLights, JointBuffers& jointBuffers, u32 windowWidth, u32 windowHeight) {
	// init for shadow render
	glUseProgram(renderer.shadowShader);
	// glBindVertexArray(renderer.shadowVao);

	// shadow render for dir light
	ShadowRender(renderer, dirLight.shadowMap, dirLight.cameraForShadows, renderObjs, nRenderObjs, jointBuffers, windowWidth, windowHeight);
	// shadow render for point / spot lights
	for(u32 i = 0; i < nLights; i++) {
		ShadowRender(renderer, lights[i].shadowMap, lights[i].cameraForShadows, renderObjs, nRenderObjs, jointBuffers, windowWidth, windowHeight);
	}

	// init for default render
	glUseProgram(renderer.shader.program);
	// glBindVertexArray(renderer.vao);

	glUniform3fv(renderer.shader.u_cameraPos, 1, &camera.pos[0]);
	glUniform3fv(renderer.shader.u_lightDir, 1, &dirLight.cameraForShadows.dir[0]);
	glUniform3fv(renderer.shader.u_lightColor, 1, &dirLight.color[0]);
	BindTexture(renderer.shader.u_shadowMap, dirLight.shadowMap.texture, 3);

    if(renderer.drawToFBO) {
    	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer.fbo.id);
    }

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for(u32 i = 0; i < nRenderObjs; i++) {
		RenderObj& obj = renderObjs[i];
		BindMaterial(renderer.shader, *renderObjs[i].material);

		if(i == 2) {
			BindTexture(renderer.shader.u_texture, dirLight.shadowMap.texture, 0);
		}

		// disable mapping for objects without that map
		GLint texture_mapping_enabled;
		glGetUniformiv(renderer.shader.program, renderer.shader.texture_mapping_enabled, &texture_mapping_enabled);
		if(renderObjs[i].material->texture == NULL)
			glUniform1i(renderer.shader.texture_mapping_enabled, 0);
		GLint normal_mapping_enabled;
		glGetUniformiv(renderer.shader.program, renderer.shader.normal_mapping_enabled, &normal_mapping_enabled);
		if(renderObjs[i].material->normalMap == NULL)
			glUniform1i(renderer.shader.normal_mapping_enabled, 0);
		GLint displacement_mapping_enabled;
		glGetUniformiv(renderer.shader.program, renderer.shader.displacement_mapping_enabled, &displacement_mapping_enabled);
		if(renderObjs[i].material->dispMap == NULL)
			glUniform1i(renderer.shader.displacement_mapping_enabled, 0);

		// update model uniforms
		glUniformMatrix4fv(renderer.shader.u_modelMatrix, 1, GL_FALSE, &obj.transform.matrix[0][0]);
		glUniformMatrix4fv(renderer.shader.u_normalMatrix, 1, GL_FALSE, &obj.normalMatrix[0][0]);
		mat4 mvpMatrix = camera.vpMatrix * obj.transform.matrix;
		glUniformMatrix4fv(renderer.shader.u_mvpMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);

		// load jointTransforms
		if(obj.model->numJoints > 0) {
			CalcJointTransforms(obj, jointBuffers);
			glUniformMatrix4fv(renderer.shader.u_jointTransforms, MAX_JOINTS_PER_MODEL, GL_FALSE, &jointBuffers.jointTransforms[0][0][0]);
    		glUniform1i(renderer.shader.skeletal_animations_enabled, 1);
		}
		else {
    		glUniform1i(renderer.shader.skeletal_animations_enabled, 0);
		}

		// update light uniforms
		mvpMatrix = dirLight.cameraForShadows.vpMatrix * obj.transform.matrix;
		glUniformMatrix4fv(renderer.shader.u_mvpMatrixFromLight, 1, GL_FALSE, &mvpMatrix[0][0]);
		// TODO: update point / spot light uniforms

		// draw the render obj
// glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDrawElements(GL_TRIANGLES, renderObjs[i].model->numIndices, GL_UNSIGNED_INT, (void*)(renderObjs[i].model->indicesOffset * sizeof(IndexType)));
// glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		// reset back to what we had for mapping
		glUniform1i(renderer.shader.normal_mapping_enabled, normal_mapping_enabled);
		glUniform1i(renderer.shader.texture_mapping_enabled, texture_mapping_enabled);
		glUniform1i(renderer.shader.displacement_mapping_enabled, displacement_mapping_enabled);
	}
    if(renderer.drawToFBO) {
    	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}
