#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"
#include "helpers/MeshUtilities.h"

#include "Plane.h"

Plane::Plane(){}

Plane::~Plane(){}

void Plane::init(GLuint shadowMapTextureId){
	
	// Load the shaders
	_programDepthId = createGLProgram("ressources/shaders/object_depth.vert","ressources/shaders/object_depth.frag");
	_programId = createGLProgram("ressources/shaders/plane.vert","ressources/shaders/plane.frag");

	// Load geometry.
	mesh_t mesh;
	loadObj("ressources/plane.obj",mesh,Indexed);
	centerAndUnitMesh(mesh);
	computeTangentsAndBinormals(mesh);
	
	_count = mesh.indices.size();

	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.positions.size() * 3, &(mesh.positions[0]), GL_STATIC_DRAW);
	
	GLuint vbo_nor = 0;
	glGenBuffers(1, &vbo_nor);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.normals.size() * 3, &(mesh.normals[0]), GL_STATIC_DRAW);
	
	GLuint vbo_uv = 0;
	glGenBuffers(1, &vbo_uv);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.texcoords.size() * 2, &(mesh.texcoords[0]), GL_STATIC_DRAW);
	
	GLuint vbo_tan = 0;
	glGenBuffers(1, &vbo_tan);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.tangents.size() * 3, &(mesh.tangents[0]), GL_STATIC_DRAW);
	
	GLuint vbo_binor = 0;
	glGenBuffers(1, &vbo_binor);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.binormals.size() * 3, &(mesh.binormals[0]), GL_STATIC_DRAW);
	
	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// The second attribute will be the normals.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// The third attribute will be the uvs.
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// The fourth attribute will be the tangents.
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// The fifth attribute will be the binormals.
	glEnableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// We load the indices data
	glGenBuffers(1, &_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
	
	// Get a binding point for the light in Uniform buffer.
	_lightUniformId = glGetUniformBlockIndex(_programId, "Light");
	
	// Load and upload the textures.
	_texColor = loadTexture("ressources/plane_texture_color.png", _programId, 0,  "textureColor", true);
	
	_texNormal = loadTexture("ressources/plane_texture_normal.png", _programId, 1, "textureNormal");
	
	_texEffects = loadTexture("ressources/plane_texture_depthmap.png", _programId, 2, "textureEffects");
	
	_texCubeMap = loadTextureCubeMap("ressources/cubemap/cubemap", _programId, 3, "textureCubeMap", true);
	
	_texCubeMapSmall = loadTextureCubeMap("ressources/cubemap/cubemap_diff", _programId, 4, "textureCubeMapSmall", true);
	
	// Load the shadowMap texture
	_shadowMapId = shadowMapTextureId;
	glBindTexture(GL_TEXTURE_2D, _shadowMapId);
	GLuint texUniID = glGetUniformLocation(_programId, "shadowMap");
	glUniform1i(texUniID, 5);
	
	checkGLError();
	
}


void Plane::draw(float elapsed, const glm::mat4& view, const glm::mat4& projection, size_t pingpong){
	
	glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	// Combine the three matrices.
	glm::mat4 MV = view * model;
	glm::mat4 MVP = projection * MV;
	
	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	// Compute the inverse view matrix
	glm::mat4 invView = glm::inverse(view);
	// Select the program (and shaders).
	glUseProgram(_programId);

	// Select the right sub-uniform buffer to use for the light.
	glUniformBlockBinding(_programId, _lightUniformId, pingpong);
	
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
	// Upload the MV matrix.
	GLuint mvID  = glGetUniformLocation(_programId, "mv");
	glUniformMatrix4fv(mvID, 1, GL_FALSE, &MV[0][0]);
	// Upload the normal matrix.
	GLuint normalMatrixID  = glGetUniformLocation(_programId, "normalMatrix");
	glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
	// Upload the inverse view matrix.
	GLuint invVID  = glGetUniformLocation(_programId, "inverseV");
	glUniformMatrix4fv(invVID, 1, GL_FALSE, &invView[0][0]);
	
	// Bind the textures.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _texColor);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _texNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, _texEffects);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMap);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	
	// Upload the light MVP matrix.
	GLuint lmvpID  = glGetUniformLocation(_programId, "lightMVP");
	glUniformMatrix4fv(lmvpID, 1, GL_FALSE, &_lightMVP[0][0]);
	
	// Bind the shadowMap texture.
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, _shadowMapId);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);

	glBindVertexArray(0);
	glUseProgram(0);
}

void Plane::drawDepth(float elapsed, const glm::mat4& vp){
	
	glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	_lightMVP = vp * model;
	
	glUseProgram(_programDepthId);
	
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programDepthId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &_lightMVP[0][0]);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
	
}



void Plane::clean(){
	glDeleteVertexArrays(1, &_vao);
}


