#ifndef SUBDIVISION_H
#define SUBDIVISION_H

#include <glm/glm.hpp>
#include <vector>

class Subdivision {
public:
	Subdivision();
	~Subdivision();

	void loop_subdivision(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) const;

  void catmull_clark_subdivision(std::vector<glm::vec4>& obj_vertices,
                       	 std::vector<glm::uvec3>& obj_faces) const;

  void show_limit_surface(std::vector<glm::vec4>& obj_vertices,
                        std::vector<glm::uvec3>& obj_faces) const;

private:
};

#endif
