#ifndef MENGER_H
#define MENGER_H

#include <glm/glm.hpp>
#include <vector>

inline void
CreateCube(std::vector<glm::vec4>& vertices,
        std::vector<glm::uvec3>& indices, float size, glm::vec3 pos)
{
	int l = vertices.size();

	for(int i = 0; i < 8; i++){
		int x = (i % 2) * 2 - 1;
		int y = ((i/2) % 2) * 2 - 1;
		int z = ((i/4) % 2) * 2 - 1;
		vertices.push_back(glm::vec4(0.5f * x * size + pos.x, 0.5f * y * size + pos.y, 0.5f * z * size + pos.z, 1.0f));
	}
	// vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
	// vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
	// vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));

	//Back
	indices.push_back(glm::uvec3(0 + l, 2 + l, 1 + l));
	indices.push_back(glm::uvec3(2 + l, 3 + l, 1 + l));

	//Front
	indices.push_back(glm::uvec3(5 + l, 7 + l, 6 + l));
	indices.push_back(glm::uvec3(4 + l, 5 + l, 6 + l));

	//Right
	indices.push_back(glm::uvec3(1 + l, 3 + l, 7 + l));
	indices.push_back(glm::uvec3(1 + l, 7 + l, 5 + l));

	//Left
	indices.push_back(glm::uvec3(0 + l, 4 + l, 6 + l));
	indices.push_back(glm::uvec3(0 + l, 6 + l, 2 + l));

	//Up
	indices.push_back(glm::uvec3(6 + l, 3 + l, 2 + l));
	indices.push_back(glm::uvec3(6 + l, 7 + l, 3 + l));

	//Down
	indices.push_back(glm::uvec3(4 + l, 0 + l, 1 + l));
	indices.push_back(glm::uvec3(4 + l, 1 + l, 5 + l));
};

class Menger {
public:
	Menger();
	~Menger();
	void set_nesting_level(int);
	bool is_dirty() const;
	void set_clean();
	void generate_geometry(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) const;
private:
	int nesting_level_ = 0;
	bool dirty_ = false;
};

#endif
