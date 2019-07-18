#include "Raycaster.hpp"
#include "helpers/Random.hpp"
#include "helpers/System.hpp"
#include <queue>
#include <stack>

Raycaster::Ray::Ray(const glm::vec3 & origin, const glm::vec3 & direction) : pos(origin), dir(glm::normalize(direction)){
}

Raycaster::RayHit::RayHit() : hit(false), dist(std::numeric_limits<float>::max()), u(0.0f), v(0.0f), localId(0), meshId(0) {
}

Raycaster::RayHit::RayHit(float distance, float uu, float vv, unsigned long lid, unsigned long mid) :
	hit(true), dist(distance), u(uu), v(vv), w(1.0f - uu - vv), localId(lid), meshId(mid) {
}

Raycaster::Raycaster(){
	
}

void Raycaster::addMesh(const Mesh & mesh, const glm::mat4 & model){
	const unsigned long indexOffset = (unsigned long)(_vertices.size());
	
	// Start by copying all vertices.
	const size_t startIndex = _vertices.size();
	_vertices.insert(_vertices.end(), mesh.positions.begin(), mesh.positions.end());
	const size_t endIndex = _vertices.size();
	
	if(model != glm::mat4(1.0f)){
		for(size_t vid = startIndex; vid < endIndex; ++vid){
			_vertices[vid] = glm::vec3(model * glm::vec4(_vertices[vid], 1.0f));
		}
	}
	
	const size_t trianglesCount = mesh.indices.size()/3;
	for(size_t tid = 0; tid < trianglesCount; ++tid){
		const size_t localId = 3 * tid;
		TriangleInfos triInfos;
		triInfos.v0 = indexOffset + mesh.indices[localId + 0];
		triInfos.v1 = indexOffset + mesh.indices[localId + 1];
		triInfos.v2 = indexOffset + mesh.indices[localId + 2];
		triInfos.localId = (unsigned long)(localId);
		triInfos.meshId = _meshCount;
		triInfos.bbox = BoundingBox(_vertices[triInfos.v0], _vertices[triInfos.v1], _vertices[triInfos.v2]);
		_triangles.push_back(triInfos);
	}
	
	Log::Info() << "[Raycaster]" << " Mesh " << _meshCount << " added, " << trianglesCount << " triangles, " << _vertices.size() - indexOffset << " vertices." << std::endl;
	
	++_meshCount;
}

void Raycaster::updateHierarchy(){
	
	Log::Info() << "[Raycaster] Building hierarchy... " << std::flush;
	
	struct SetInfos {
		size_t begin;
		size_t count;
		long parent;
		bool right;
	};
	
	std::stack<SetInfos> remainingSets;
	remainingSets.push({0, _triangles.size(), -1, false});
	
	while(!remainingSets.empty()){
		// Get the next node to process on the stack.
		const SetInfos current(remainingSets.top());
		remainingSets.pop();
		const size_t begin = current.begin;
		const size_t count = current.count;
		
		// Compute the global bounding box.
		BoundingBox global(_triangles[begin].bbox);
		for(size_t tid = 1; tid < count; ++tid){
			global.merge(_triangles[begin+tid].bbox);
		}
		// Pick the dimension along which the global bbox is the largest.
		const glm::vec3 bboxSize = global.getSize();
		const int axis = (bboxSize.x >= bboxSize.y && bboxSize.x >= bboxSize.z) ? 0 : (bboxSize.y >= bboxSize.z ? 1 : 2);
		// Sort all triangles along the picked axis.
		std::sort(_triangles.begin()+begin, _triangles.begin()+begin+count, [axis](const TriangleInfos & t0, const TriangleInfos & t1){
			return t0.bbox.minis[axis] < t1.bbox.minis[axis];
		});
		
		// Create the node.
		_hierarchy.emplace_back();
		const size_t nodeId = _hierarchy.size()-1;
		Node currentNode;
		currentNode.box = global;
		// If the triangles count is low enough, we have a leaf.
		if(count < 3){
			currentNode.leaf = true;
			currentNode.left = begin;
			currentNode.right = count;
		} else {
			currentNode.leaf = false;
			// Create the two sub-nodes.
			remainingSets.push({begin, count/2, long(nodeId), false});
			remainingSets.push({begin+count/2, count-count/2, long(nodeId), true});
		}
		_hierarchy[nodeId] = currentNode;
		
		// Update the parent node with infos.
		if(current.parent >= 0){
			if(current.right){
				_hierarchy[current.parent].right = nodeId;
			} else {
				_hierarchy[current.parent].left = nodeId;
			}
		}
		
	}
	Log::Info() << "Done." << std::endl;
}

void Raycaster::createBVHMeshes(std::vector<Mesh> &meshes) const {
	meshes.clear();
	// We want a mesh where the nodes are in increasing order of depth.
	struct NodeLocation {
		size_t node;
		size_t depth;
	};
	
	std::vector<NodeLocation> sortedNodes;
	sortedNodes.reserve(_hierarchy.size());
	
	// Breadth-first tree exploration.
	std::queue<NodeLocation> nodesToVisit;
	nodesToVisit.push({0, 0});
	size_t maxDepth = 0;
	while(!nodesToVisit.empty()){
		const NodeLocation & location = nodesToVisit.front();
		sortedNodes.push_back(location);
		// If this is not a leaf, enqueue the two children nodes.
		const Node & node = _hierarchy[location.node];
		if(!node.leaf){
			nodesToVisit.push({ node.left , location.depth+1 });
			nodesToVisit.push({ node.right, location.depth+1 });
		}
		// Find the max depth.
		maxDepth = std::max(maxDepth, location.depth);
		// Remove the current node from the visit queue.
		nodesToVisit.pop();
	}
	
	meshes.resize(maxDepth+1);
	
	// For each node, generate a wireframe bounding box.
	for(const auto & location : sortedNodes){
		const Node & node = _hierarchy[location.node];
		
		// Compute relative depth for colorisation.
		float depth = float(location.depth) / float(maxDepth);
		// We have fewer boxes at low depth, skew the hue scale.
		depth *= depth;
		// Decrease luminosity as we go deeper.
		const float lum = 0.5*(1.0f - depth);
		const glm::vec3 color = System::hslToRgb(glm::vec3(300.0f*depth, 0.9f, lum));
		
		// Setup vertices.
		Mesh & mesh = meshes[location.depth];
		const size_t firstIndex = mesh.positions.size();
		const auto corners = node.box.getCorners();
		for(const auto & corner : corners){
			mesh.positions.push_back(corner);
			mesh.colors.push_back(color);
		}
		// Setup degenerate triangles for each line.
		const std::vector<unsigned int> indices = {
			0,1,0, 0,2,0, 1,3,1, 2,3,2, 4,5,4, 4,6,4, 5,7,5, 6,7,6, 1,5,1, 0,4,0, 2,6,2, 3,7,3
		};
		for(const int iid : indices){
			mesh.indices.push_back(firstIndex + iid);
		}
	}
}

const Raycaster::RayHit Raycaster::intersects(const glm::vec3 & origin, const glm::vec3 & direction, float mini, float maxi) const {
	const Ray ray(origin, direction);
	
	std::stack<size_t> nodesToTest;
	nodesToTest.push(0);
	
	RayHit bestHit;
	while(!nodesToTest.empty()){
		const Node & node = _hierarchy[nodesToTest.top()];
		nodesToTest.pop();
		
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, node.box, mini, maxi)){
			continue;
		}
		// If the node is a leaf, test all included triangles.
		if(node.leaf){
			for(size_t tid = 0; tid < node.right; ++tid){
				const auto & tri = _triangles[node.left + tid];
				const RayHit hit = intersects(ray, tri, mini, maxi);
				// We found a valid hit.
				if(hit.hit && hit.dist < bestHit.dist){
					bestHit = hit;
					maxi = bestHit.dist;
				}
			}
			// Move to the next node.
			continue;
		}
		// Else, intersect both child nodes.
		nodesToTest.push(node.left);
		nodesToTest.push(node.right);
	}
	return bestHit;
}

bool Raycaster::intersectsAny(const glm::vec3 & origin, const glm::vec3 & direction, float mini, float maxi) const {
	const Ray ray(origin, direction);
	
	std::stack<size_t> nodesToTest;
	nodesToTest.push(0);
	
	while(!nodesToTest.empty()){
		const Node & node = _hierarchy[nodesToTest.top()];
		nodesToTest.pop();
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, node.box, mini, maxi)){
			continue;
		}
		// If the node is a leaf, test all included triangles.
		if(node.leaf){
			RayHit finalHit;
			for(size_t tid = 0; tid < node.right; ++tid){
				const auto & tri = _triangles[node.left + tid];
				if(intersects(ray, tri, mini, maxi).hit){
					return true;
				}
			}
			// No intersection move to the next node.
			continue;
		}
		// Check if any of the children is hit.
		nodesToTest.push(node.left);
		nodesToTest.push(node.right);
	}
	return false;
}

bool Raycaster::visible(const glm::vec3 & p0, const glm::vec3 & p1) const {
	const glm::vec3 direction = p1 - p0;
	const float maxi = glm::length(direction);
	return !intersectsAny(p0, direction, 0.0001f, maxi);
}

const Raycaster::RayHit Raycaster::intersects(const Raycaster::Ray & ray, const TriangleInfos & tri, float mini, float maxi) const {
	// Implement Moller-Trumbore intersection test.
	const glm::vec3 & v0 = _vertices[tri.v0];
	const glm::vec3 v01 = _vertices[tri.v1] - v0;
	const glm::vec3 v02 = _vertices[tri.v2] - v0;
	const glm::vec3 p = glm::cross(ray.dir, v02);
	const float det = glm::dot(v01, p);
	
	if(std::abs(det) < 0.00001f){
		return RayHit();
	}
	
	const float invDet = 1.0f / det;
	const glm::vec3 q = ray.pos - v0;
	const float u = invDet * glm::dot(q, p);
	if(u < 0.0f || u > 1.0f){
		return RayHit();
	}
	
	const glm::vec3 r = glm::cross(q, v01);
	const float v = invDet * glm::dot(ray.dir, r);
	if(v < 0.0f || (u+v) > 1.0f){
		return RayHit();
	}
	
	const float t = invDet * glm::dot(v02, r);
	if(t > mini && t < maxi){
		return RayHit(t, u, v, tri.localId, tri.meshId);
	}
	return RayHit();
}

bool Raycaster::intersects(const Raycaster::Ray & ray, const BoundingBox & box, float mini, float maxi){
	const glm::vec3 minRatio = (box.minis - ray.pos) / ray.dir;
	const glm::vec3 maxRatio = (box.maxis - ray.pos) / ray.dir;
	const glm::vec3 minFinal = glm::min(minRatio, maxRatio);
	const glm::vec3 maxFinal = glm::max(minRatio, maxRatio);
	
	const float closest  = std::max(minFinal[0], std::max(minFinal[1], minFinal[2]));
	const float furthest = std::min(maxFinal[0], std::min(maxFinal[1], maxFinal[2]));
	
	return std::max(closest, mini) <= std::min(furthest, maxi);
}

glm::vec3 Raycaster::interpolatePosition(const RayHit & hit, const Mesh & geometry){
	const unsigned long triId = hit.localId;
	const unsigned long i0 = geometry.indices[triId  ];
	const unsigned long i1 = geometry.indices[triId+1];
	const unsigned long i2 = geometry.indices[triId+2];
	return hit.w * geometry.positions[i0] + hit.u * geometry.positions[i1] + hit.v * geometry.positions[i2];
}

glm::vec3 Raycaster::interpolateNormal(const RayHit & hit, const Mesh & geometry){
	const unsigned long triId = hit.localId;
	const unsigned long i0 = geometry.indices[triId  ];
	const unsigned long i1 = geometry.indices[triId+1];
	const unsigned long i2 = geometry.indices[triId+2];
	const glm::vec3 n = hit.w * geometry.normals[i0] + hit.u * geometry.normals[i1] + hit.v * geometry.normals[i2];
	return glm::normalize(n);
}

glm::vec2 Raycaster::interpolateUV(const RayHit & hit, const Mesh & geometry){
	const unsigned long triId = hit.localId;
	const unsigned long i0 = geometry.indices[triId  ];
	const unsigned long i1 = geometry.indices[triId+1];
	const unsigned long i2 = geometry.indices[triId+2];
	return hit.w * geometry.texcoords[i0] + hit.u * geometry.texcoords[i1] + hit.v * geometry.texcoords[i2];
}
