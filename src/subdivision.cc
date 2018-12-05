#include "subdivision.h"
#include <algorithm>

Subdivision::Subdivision()
{
	// Add additional initialization if you like
}

Subdivision::~Subdivision()
{
}

void Subdivision::loop_subdivision(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) 
{
	std::cout << "test" << std::endl;
	as = new AdjacencyStructure(obj_vertices, obj_faces);
	//as->printStructure();
	std::vector<glm::vec4> odd_vertices;
	for(auto face : obj_faces){
		std::vector<int> combination{0,1,2};
		for(auto i : combination){
			std::vector<int> current = combination;
			current.erase(current.begin() + i);

			std::set<int> sf = shared_faces(face[current[0]], face[current[1]]);
			std::set<int> outer_vertices;
			for(auto f : sf){
				outer_vertices.insert(obj_faces[f].x);
				outer_vertices.insert(obj_faces[f].y);
				outer_vertices.insert(obj_faces[f].z);
			}
			outer_vertices.erase(face[current[0]]);
			outer_vertices.erase(face[current[1]]);

			// for(auto v: outer_vertices){
			// 	std::cout << v << " ";
			// }
			// std::cout << std::endl;
			int outer_v0 = *outer_vertices.begin();
			int outer_v1 = *(++outer_vertices.begin());
			glm::vec4 new_odd_vertix = (0.375f)*(obj_vertices[face[current[0]]] + obj_vertices[face[current[1]]]) + (0.125f)*(obj_vertices[outer_v0] + obj_vertices[outer_v1]);
			odd_vertices.push_back(new_odd_vertix);
		}



	}



}

void Subdivision::catmull_clark_subdivision(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) const
{
}

void Subdivision::show_limit_surface(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) const
{
}

std::set<int> Subdivision::shared_faces(int v1, int v2) const
{
	std::set<int> intersect;
	std::set_intersection(as->vertices[v1].adj_faces.begin(), as->vertices[v1].adj_faces.end(), as->vertices[v2].adj_faces.begin(), as->vertices[v2].adj_faces.end(),
	 std::inserter(intersect, intersect.begin()));

	// for (auto face: intersect) {
	// 	std::cout << face << std::endl;
	// }

	return intersect;


}
