#include "processing/SSAO.hpp"
#include "generation/Random.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"

SSAO::SSAO(uint width, uint height, uint downscale, float radius) : _mediumBlur(true),
	_samples(16, BufferType::UNIFORM, DataUse::STATIC), _radius(radius), _downscale(downscale) {

	const Descriptor desc = Descriptor(Layout::R8, Filter::LINEAR_NEAREST, Wrap::CLAMP);
	_ssaoFramebuffer.reset(new Framebuffer(width/_downscale, height/_downscale, desc, false, "SSAO"));
	_finalFramebuffer.reset(new Framebuffer(width, height, desc, false, "SSAO final"));
	_programSSAO = Resources::manager().getProgram2D("ssao");

	// Generate samples.
	// We need random vectors in the half sphere above z, with more samples close to the center.
	for(int i = 0; i < 16; ++i) {
		const glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
			Random::Float(-1.0f, 1.0f),
			Random::Float(0.0f, 1.0f));
		_samples[i] = glm::vec4(glm::normalize(randVec), 0.0f);
		_samples[i] *= Random::Float(0.0f, 1.0f);
		// Skew the distribution towards the center.
		float scale = float(i) / 16.0f;
		scale		= 0.1f + 0.9f * scale * scale;
		_samples[i] *= scale;
	}
	// Send the samples to the GPU.
	_samples.setup();
	_samples.upload();

	// Noise texture (same size as the box blur applied after SSAO computation).
	// We need to generate two dimensional normalized offsets.
	_noisetexture.width  = 5;
	_noisetexture.height = 5;
	_noisetexture.depth  = 1;
	_noisetexture.levels = 1;
	_noisetexture.shape  = TextureShape::D2;
	_noisetexture.images.emplace_back();
	Image & img	= _noisetexture.images.back();
	img.width	  = 5;
	img.height	 = 5;
	img.components = 3;

	for(int i = 0; i < 25; ++i) {
		const glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
			Random::Float(-1.0f, 1.0f),
			0.0f);
		const glm::vec3 norVec  = glm::normalize(randVec);
		img.pixels.push_back(norVec[0]);
		img.pixels.push_back(norVec[1]);
		img.pixels.push_back(norVec[2]);
	}

	// Send the texture to the GPU.
	_noisetexture.upload({Layout::RGB32F, Filter::NEAREST, Wrap::REPEAT}, false);

	checkGLError();
}

// Draw function
void SSAO::process(const glm::mat4 & projection, const Texture * depthTex, const Texture * normalTex) {

	GLUtilities::setDepthState(false);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true, Faces::BACK);

	_ssaoFramebuffer->bind();
	_ssaoFramebuffer->setViewport();
	_programSSAO->use();
	_programSSAO->uniform("projectionMatrix", projection);
	_programSSAO->uniform("radius", _radius);
	GLUtilities::bindBuffer(_samples, 0);
	ScreenQuad::draw({depthTex, normalTex, &_noisetexture});

	// Blurring pass
	if(_quality == Quality::HIGH){
		_highBlur.process(projection, _ssaoFramebuffer->texture(), depthTex, normalTex, *_finalFramebuffer);
	} else if(_quality == Quality::MEDIUM){
		// Render at potentially low res.
		_mediumBlur.process(_ssaoFramebuffer->texture(), *_ssaoFramebuffer);
		GLUtilities::blit(*_ssaoFramebuffer, *_finalFramebuffer, Filter::LINEAR);
	} else {
		GLUtilities::blit(*_ssaoFramebuffer, *_finalFramebuffer, Filter::LINEAR);
	}
}

void SSAO::clear() const {
	_finalFramebuffer->clear(glm::vec4(1.0f), 1.0f);
}

// Handle screen resizing
void SSAO::resize(uint width, uint height) const {
	_ssaoFramebuffer->resize(width/_downscale, height/_downscale);
	_finalFramebuffer->resize(width, height);
	// The blurs resize automatically.
}

const Texture * SSAO::texture() const {
	return _finalFramebuffer->texture();
}

float & SSAO::radius() {
	return _radius;
}

SSAO::Quality & SSAO::quality() {
	return _quality;
}

SSAO::~SSAO() {
	_noisetexture.clean();
}
