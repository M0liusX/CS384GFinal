#include "subdivision.h"
#include <iostream>

Subdivision::Subdivision()
{
	// Add additional initialization if you like
}

Subdivision::~Subdivision()
{
}

void Subdivision::loop_subdivision(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) const
{
}

void Subdivision::catmull_clark_subdivision(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) const
{
}

void Subdivision::show_limit_surface(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) const
{
}
