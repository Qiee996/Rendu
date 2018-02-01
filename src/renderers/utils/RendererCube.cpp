#include "RendererCube.hpp"
#include "../../input/Input.hpp"

#include <stdio.h>
#include <iostream>
#include <vector>
#ifdef _WIN32
#define M_PI_2 1.57079632679
#endif

RendererCube::~RendererCube(){}

RendererCube::RendererCube(Config & config, const std::string & cubemapName, const std::string & shaderName, const int width, const int height, const GLenum format, const GLenum type, const GLenum preciseFormat, const std::string & outputPath) : Renderer(config) {
	
	_outputPath = outputPath;
	
	_resultFramebuffer = std::make_shared<Framebuffer>(width, height, format, type, preciseFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	
	
	_program = Resources::manager().getProgram(shaderName, "object_basic", shaderName);
	_cubemap = Object(_program, "skybox", {}, {{cubemapName, true }});
	
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	checkGLError();
	
	checkGLError();
	
}


void RendererCube::draw() {
	int count = 0;
	for(float rr = 0.0f; rr < 1.1f; rr += 0.2f){
		glUseProgram(_program->id());
		glUniform1f(_program->uniform("mimapRoughness"), rr);
		glUseProgram(0);
		const int powe = (int)std::pow(2, count);
		const int localWidth = _resultFramebuffer->width()/powe;
		const int localHeight = _resultFramebuffer->height()/powe;
		
		const GLenum type = _resultFramebuffer->type();
		const GLenum format = _resultFramebuffer->format();
		const unsigned int components = (format == GL_RED ? 1 : (format == GL_RG ? 2 : (format == GL_RGB ? 3 : 4)));
		
		_resultFramebuffer->bind();
		glViewport(0,0,localWidth,localHeight);
		
		const glm::mat4 projection = glm::perspective((float)M_PI_2, (float)_resultFramebuffer->width()/(float)_resultFramebuffer->height(), 0.1f, 200.0f);
		
		// Loop over the faces. Instead we could use a geometry shader and multiple output layers, one for each face with the corresponding view transformation.
		const glm::vec3 ups[6] = { glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0),glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0) };
		const glm::vec3 centers[6] = { glm::vec3(1.0,0.0,0.0), glm::vec3(-1.0,0.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0), glm::vec3(0.0,1.0,0.0), glm::vec3(0.0,-1.0,0.0) };
		const std::string suffixes[6] = { "nx", "px", "nz", "pz", "py", "ny"};
		
		for(size_t i = 0; i < 6; ++i){
			const glm::mat4 view = glm::lookAt(glm::vec3(0.0f,0.0f,0.0f), centers[i], ups[i]);
			
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			_cubemap.draw(view, projection);
			glFlush();
			glFinish();
			
			if(type == GL_FLOAT){
				// Get back values.
				float * data = new float[localWidth*localHeight*components];
				glReadPixels(0,0,localWidth,localHeight, format, type, &data[0]);
				// Save data.
				GLUtilities::saveTexture(_outputPath + "-" + std::to_string(rr) + "-" + suffixes[i] +  ".exr" , localWidth,localHeight, components, (void*)data, true);
				data = NULL;
			} else if (type == GL_UNSIGNED_BYTE){
				// Get back values.
				GLubyte * data = new GLubyte[localWidth*localHeight*components];
				glReadPixels(0,0, localWidth,localHeight, format, type, &data[0]);
				// Save data.
				GLUtilities::saveTexture(_outputPath + "-" + std::to_string(rr) + "-" + suffixes[i] +  ".png",  localWidth,localHeight, components, (void*)data, false);
				delete[] data; data = NULL;
			}
		}
		
		_resultFramebuffer->unbind();
		
		++count;
	}
}

void RendererCube::update(){
	Renderer::update();
	// Update nothing.
}

void RendererCube::physics(double fullTime, double frameTime){
	// Simulate nothing.
}


void RendererCube::clean() const {
	Renderer::clean();
	// Clean objects.
	_cubemap.clean();
	_resultFramebuffer->clean();
	
}


void RendererCube::resize(int width, int height){
	// Do nothing.
}


