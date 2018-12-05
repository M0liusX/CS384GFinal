#ifndef SUBDIVISION_H
#define SUBDIVISION_H

#include <glm/glm.hpp>
#include <vector>
#include <set>
#include <iostream>

class Vertex {
public:
	int id;
	std::vector<int> adj_faces;
	std::set<int> adj_vertices;

};

/*class Face {
	int id;
	vector<int> adj_vertices;
}*/

class AdjacencyStructure {
public:
	std::vector<Vertex> vertices;

	AdjacencyStructure(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) {
		vertices = std::vector<Vertex>(obj_vertices.size());
		for (int i = 0; i < obj_vertices.size(); ++i)
		{
			vertices[i].id = i;
		}

		for (int i = 0; i < obj_faces.size(); ++i)
		{
			vertices[obj_faces[i].x].adj_faces.push_back(i);
			vertices[obj_faces[i].y].adj_faces.push_back(i);
			vertices[obj_faces[i].z].adj_faces.push_back(i);
		}

		for (int i = 0; i < vertices.size(); ++i) {
			for (auto face : vertices[i].adj_faces) {
				vertices[i].adj_vertices.insert(obj_faces[face].x);
				vertices[i].adj_vertices.insert(obj_faces[face].y);
				vertices[i].adj_vertices.insert(obj_faces[face].z);
			}
			//std::cout << vertex.adj_vertices.size() << " ";
			vertices[i].adj_vertices.erase(vertices[i].id);
			//std::cout << vertex.adj_vertices.size() << " ";
		}

	}

	void printStructure() {
		for (auto vertex : vertices) {
			std::cout << "v" << vertex.id + 1 << ": ";
			for (auto v : vertex.adj_vertices)
			{
				std::cout << v << " ";
			}
			std::cout << std::endl;
		}
	}
};

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

  void outer(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) const;

private:
};

#endif
