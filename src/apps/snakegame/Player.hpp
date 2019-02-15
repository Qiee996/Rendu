#ifndef Player_h
#define Player_h
#include "Common.hpp"
#include "resources/ResourcesManager.hpp"

class Player {
public:
	Player();
	
	void update();
	
	void physics(double fullTime, double frameTime);
	
	void draw(const glm::mat4& view, const glm::mat4& projection) ;
	
	void clean() const;
	
private:
	
	std::shared_ptr<ProgramInfos> _coloredProgram;
	
	MeshInfos _head;
	MeshInfos _bodyElement;
	
	glm::vec3 _momentum = glm::vec3(0.0f, 1.0f, 0.0f);
	
	glm::vec3 _position = glm::vec3(0.0f);
	float _angle = 0.0f;
	std::vector<glm::vec2> _positions;
	std::vector<float> _angles;
	std::vector<glm::vec2> _items;
	
	struct PathPoint {
		glm::vec2 pos;
		float dist;
	};
	std::vector<PathPoint> _path;
	unsigned int _currentSample = 0;
	unsigned int _currentFrame = 0;
	
	double _lastSpawn = 0.0;
	float _invicibility = 0.0f;
	int _score = 0;
	
	const glm::vec3 _maxPos = glm::vec3(8.6f, 5.0f, 0.0f);
	const double _spawnPeriod = 1.5;
	const size_t _numSamplesPath = 512;
	const size_t _samplingPeriod = 15;
	const float _radius = 0.5f;
	const float _headAccel = 400.0f;
	const float _angleSpeed = 6.0f;
	const float _minSamplingDistance = 0.02f;
	const float _invicibilityIncrease = 0.5f;
	const float _eatingDistance = 1.5f;
	const float _minSpawnDistance = 3.0f;
	const float _collisionDistance = 1.5f;
	const int _spawnTentatives = 50;
	const int _maxItems = 20;
	const int _itemValue = 1;
};

#endif