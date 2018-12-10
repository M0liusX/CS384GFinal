#ifndef SUBDIVISION_H
#define SUBDIVISION_H

#include <glm/glm.hpp>
#include <vector>
#include <set>
#include <iostream>

class Vertex {
public:
	int id;
	std::set<int> adj_faces;
	std::set<int> adj_vertices;

};

class Face {
public:
	int id;
	std::vector<int> updated_vertices;
	std::vector<int> edge_vertices;
};

class AdjacencyStructure {
public:
	std::vector<Vertex> vertices;
	std::vector<Face> faces;

	AdjacencyStructure(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) {
		vertices = std::vector<Vertex>(obj_vertices.size());
		faces = std::vector<Face>(obj_faces.size());
		for (int i = 0; i < obj_vertices.size(); ++i)
		{
			vertices[i].id = i;
		}
		for (int i = 0; i < obj_faces.size(); ++i)
		{
			faces[i].id = i;
		}

		for (int i = 0; i < obj_faces.size(); ++i)
		{
			vertices[obj_faces[i].x].adj_faces.insert(i);
			vertices[obj_faces[i].y].adj_faces.insert(i);
			vertices[obj_faces[i].z].adj_faces.insert(i);
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

	AdjacencyStructure* as;

	Subdivision();
	~Subdivision();

  void loop_subdivision(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) ;

  void catmull_clark_subdivision(std::vector<glm::vec4>& obj_vertices,
                       	 std::vector<glm::uvec3>& obj_faces) const;

  void show_limit_surface(std::vector<glm::vec4>& obj_vertices,
                        std::vector<glm::uvec3>& obj_faces) const;

  std::set<int> shared_faces(int v1, int v2) const;

private:
};

#endif
