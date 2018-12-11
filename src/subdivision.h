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
	std::set<int> adj_edges;
	std::set<int> adj_vertices;

};

class Edge {
	public:
		int id;
		int v0;
		int v1;
	Edge(int idx, int u0, int u1){
		id = idx;
		v0 = u0;
		v1 = u1;
	}

};

class Face {
public:
	int id;
	std::vector<int> updated_vertices;
	std::vector<int> edge_vertices;
};

class QuadAdjacencyStructure{
public:
	std::vector<Vertex> vertices;
	std::vector<Edge> edges;
	std::vector<Face> faces;

	QuadAdjacencyStructure(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec4>& obj_faces) {
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
			vertices[obj_faces[i].w].adj_faces.insert(i);
		}

		for (int i = 0; i < vertices.size(); ++i) {
			for (auto face : vertices[i].adj_faces) {
				vertices[i].adj_vertices.insert(obj_faces[face].x);
				vertices[i].adj_vertices.insert(obj_faces[face].y);
				vertices[i].adj_vertices.insert(obj_faces[face].z);
				vertices[i].adj_vertices.insert(obj_faces[face].w);

				if(vertices[i].id == obj_faces[face].x){
					vertices[i].adj_vertices.erase(obj_faces[face].z);
				}
				if(vertices[i].id == obj_faces[face].y){
					vertices[i].adj_vertices.erase(obj_faces[face].w);
				}
				if(vertices[i].id == obj_faces[face].z){
					vertices[i].adj_vertices.erase(obj_faces[face].x);
				}
				if(vertices[i].id == obj_faces[face].w){
					vertices[i].adj_vertices.erase(obj_faces[face].y);
				}

			}
			//std::cout << vertex.adj_vertices.size() << " ";
			vertices[i].adj_vertices.erase(vertices[i].id);
			//std::cout << vertex.adj_vertices.size() << " ";
		}

		for (int i = 0; i < vertices.size(); ++i) {
			for (auto av : vertices[i].adj_vertices){
				int v0 = (i < av) ? i : av;
				int v1 = (i < av) ? av : i;
				bool unique = true;
				int idx = 0;

				for(auto edge : edges){


					if(edge.v0 == v0 && edge.v1 == v1){
						unique = false;
						break;
					}
					idx++;
				}
				if(unique){
					vertices[i].adj_edges.insert(edges.size());
					edges.emplace_back(Edge(edges.size(), v0, v1));
				} else{
					vertices[i].adj_edges.insert(idx);	
				}
				 

			}
		}

	}

	void printStructure() {
		for (auto edge : edges) {
			std::cout << "e" << edge.id + 1 << ": ";
			std::cout << edge.v0 << " " << edge.v1 << std::endl;
		}
		for (auto vertex : vertices) {
			std::cout << "v" << vertex.id + 1 << ": ";
			for (auto e : vertex.adj_edges)
			{
				std::cout << e + 1 << " ";
			}
			std::cout << std::endl;
		}
	}

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
	QuadAdjacencyStructure* qas;

	Subdivision();
	~Subdivision();

  void loop_subdivision(std::vector<glm::vec4>& obj_vertices,
	                       std::vector<glm::uvec3>& obj_faces) ;

  void catmull_clark_subdivision(std::vector<glm::vec4>& obj_vertices,
                       	 std::vector<glm::uvec4>& obj_faces,
                       	 std::vector<int>& sharp_vertices, std::vector<std::vector<int>>& creases) ;

  void show_limit_surface(std::vector<glm::vec4>& obj_vertices,
                        std::vector<glm::uvec3>& obj_faces) const;

  std::set<int> shared_faces(int v1, int v2) const;

private:
};

#endif
