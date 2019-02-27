
#include "GameMenuRenderer.hpp"
#include "Common.hpp"
#include "input/Input.hpp"

GameMenuRenderer::GameMenuRenderer(RenderingConfig & config) : Renderer(config){
	
	_buttonProgram = Resources::manager().getProgram("menu_button");
	_backgroundProgram = Resources::manager().getProgram2D("passthrough");
	_imageProgram = Resources::manager().getProgram("menu_image");
	_button = Resources::manager().getMesh("rounded-button-out");
	_buttonIn = Resources::manager().getMesh("rounded-button-in");
	_quad = Resources::manager().getMesh("plane");
}

void GameMenuRenderer::draw(const GameMenu & menu){
	
	static const std::map<MenuButton::State, glm::vec4> borderColors = {
		{ MenuButton::OFF, glm::vec4(0.9f, 0.9f, 0.9f, 1.0f) },
		{ MenuButton::HOVER, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ MenuButton::ON, glm::vec4(0.95f, 0.95f , 0.95f, 1.0f) }
	};
	
	static const std::map<MenuButton::State, glm::vec4> innerColors = {
		{ MenuButton::OFF, glm::vec4(0.9f, 0.9f, 0.9f, 0.5f) },
		{ MenuButton::HOVER, glm::vec4(1.0f, 1.0f, 1.0f, 0.5f) },
		{ MenuButton::ON, glm::vec4(0.95f, 0.95f , 0.95f, 0.5f) }
	};
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Background image.
	glUseProgram(_backgroundProgram->id());
	ScreenQuad::draw(menu.backgroundImage);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	
	// Images.
	glUseProgram(_imageProgram->id());
	for(const auto & image : menu.images){
		
		glUniform2fv(_imageProgram->uniform("position"), 1, &image.pos[0]);
		glUniform2fv(_imageProgram->uniform("scale"), 1, &image.scale[0]);
		
		glUniform1f(_imageProgram->uniform("depth"), 0.95f);
		glActiveTexture(GL_TEXTURE0 );
		glBindTexture(GL_TEXTURE_2D, image.tid);
		
		glBindVertexArray(_quad.vId);
		glDrawElements(GL_TRIANGLES, _quad.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	// Buttons
	glUseProgram(_buttonProgram->id());
	for(const auto & button : menu.buttons){
		
		glUniform2fv(_buttonProgram->uniform("position"), 1, &button.pos[0]);
		glUniform2fv(_buttonProgram->uniform("scale"), 1, &button.scale[0]);
		
		glUniform1f(_buttonProgram->uniform("depth"), 0.5f);
		glUniform4fv(_buttonProgram->uniform("color"), 1, &innerColors.at(button.state)[0]);
		glBindVertexArray(_buttonIn.vId);
		glDrawElements(GL_TRIANGLES, _buttonIn.count, GL_UNSIGNED_INT, (void*)0);
		
		glUniform1f(_buttonProgram->uniform("depth"), 0.9f);
		glUniform4fv(_buttonProgram->uniform("color"), 1, &borderColors.at(button.state)[0]);
		glBindVertexArray(_button.vId);
		glDrawElements(GL_TRIANGLES, _button.count, GL_UNSIGNED_INT, (void*)0);
		
	}
	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	checkGLError();
}

void GameMenuRenderer::update(){
	Renderer::update();
	
}

void GameMenuRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
}

void GameMenuRenderer::clean() const {
	Renderer::clean();
}

glm::vec2 GameMenuRenderer::getButtonSize(){
	return glm::vec2(_button.bbox.getSize());
}


