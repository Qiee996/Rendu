#pragma once
#include "scene/Codable.hpp"
#include "scene/Object.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/SpotLight.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"



/**
 \brief Represents a 3D environment composed of objects, a background and additional environment lighting informations.
 \ingroup Scene
 */
class Scene {

public:
	
	/** Constructor from a scene description file. The expected format is as follow:
	 \verbatim
	 scene:
	 	probe: texturetype: texturename (see Codable::decodeTexture for details)
		irradiance: shcoeffsfilename
		translation: X,Y,Z
		scaling: scale
		orientation: axisX,axisY,axisZ angle
		background: ...
	 object:
		... (see Object::decode documentation)
	 ...
	 lighttype:
		... (see Light::decode documentation)
	 ...
	 \endverbatim
	  where lighttype is one of point, directional, spot and background is one of
	 \verbatim
	 bgcolor: R,G,B
	 bgimage: texturetype: texturename
	 bgskybox: texturetype: texturename
	 \endverbatim
	
	 \param name the name of the scene description file
	 */
	Scene(const std::string & name);
	
	/** Performs initialization against the graphics API, loading data.
	 */
	void init();
	
	/** Update the animations in the scene.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 */
	void update(double fullTime, double frameTime);
	
	/** Clean internal resources. */
	void clean();
	
	std::vector<Object> objects; ///< The objects in the scene.
	std::vector<std::shared_ptr<Light>> lights; ///< Lights present in the scene.
	
	/** \brief The background mode to use for a scene. */
	enum class Background {
		COLOR, ///< Use a unique color as background.
		IMAGE, ///< Use a 2D texture image as background (will be stretched).
		SKYBOX ///< Use a skybox/cubemap as background.
	};
	Background backgroundMode = Background::COLOR; ///< The background mode (see enum).
	glm::vec3 backgroundColor = glm::vec3(0.0f); ///< Color to use if the background mode is COLOR.
	Object background; ///< Background object, containing the geometry and optional textures to use.
	
	std::vector<glm::vec3> backgroundIrradiance; ///< RGB SH-coefficients of the background irradiance, computed using SHExtractor. \see SphericalHarmonics
	const TextureInfos * backgroundReflection = nullptr; ///< Cubemap texture ID of the background radiance.
	
private:
	
	/** Load an object in the scene from its serialized representation.
	 \param params the object parameters
	 */
	void loadObject(const std::vector<KeyValues> & params);
	
	/** Load a point light in the scene from its serialized representation.
	 \param params the point light parameters
	 */
	void loadLight(const std::vector<KeyValues> & params);
	
	/** Load the scene environment informations from its serialized representation.
	 \param params the scene parameters
	 */
	void loadScene(const std::vector<KeyValues> & params);
	
	/** Load a file containing some SH coefficients approximating background irradiance.
	 \param name the name of the text file
	 \see SphericalHarmonics
	 */
	void loadSphericalHarmonics(const std::string & name);
	
	/** Compute the bounding box of the scene, optionaly excluding objects that do not cast shadows.
	 \param onlyShadowCasters denote if only objects that are allowed to cast shadows should be taken into account
	 \return the scene bounding box
	 */
	BoundingBox computeBoundingBox(bool onlyShadowCasters = false);
	
	glm::mat4 _sceneModel = glm::mat4(1.0f); ///< The scene global transformation.
	std::string _name; ///< The scene file name.
	bool _loaded = false; ///< Has the scene already been loaded from disk.
	
	
};
