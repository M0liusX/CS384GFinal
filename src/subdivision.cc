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
	//std::cout << "test" << std::endl;
	as = new AdjacencyStructure(obj_vertices, obj_faces);
	//as->printStructure();
	std::vector<glm::vec4> odd_vertices;
  std::vector<glm::vec4> even_vertices;

  //computes odd vertices
	for (int x = 0; x < obj_faces.size(); x++) {
    glm::uvec3 face = obj_faces[x];
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

      bool duplicate = false;
      int index;

      for (size_t b = 0; b < odd_vertices.size(); b++) {
        if (odd_vertices[b] == new_odd_vertix) {
          index = b;
          duplicate = true;
          //std::cout << "oh no" << '\n';
        }
      }
      if (!duplicate) {
        odd_vertices.push_back(new_odd_vertix);
        index = odd_vertices.size() - 1;
      }

      as->faces[x].edge_vertices.push_back(index);
		}
	}

  //computes new even vertices
  for (size_t z = 0; z < obj_faces.size(); z++) {
    /* code */
    for (size_t i = 0; i < 3; i++) {
      Vertex current = as->vertices[obj_faces[z][i]];
      int n = current.adj_vertices.size();
      float beta;
      glm::vec4 sum = glm::vec4(0.0, 0.0, 0.0, 0.0);

      if (n == 3) {
        beta = 3.0 / 16.0;
      }
      else {
        beta = 3.0 / (8.0 * n);
      }

      for (auto v : current.adj_vertices) {
        sum = sum + obj_vertices[v];
      }

      glm::vec4 new_even_vertex = (obj_vertices[obj_faces[z][i]] * (1 - (n * beta))) + sum * beta;

      bool duplicate = false;
      int index;

      for (size_t b = 0; b < even_vertices.size(); b++) {
        if (even_vertices[b] == new_even_vertex) {
          index = b;
          duplicate = true;
          //std::cout << "oh no" << '\n';
        }
      }
      if (!duplicate) {
        even_vertices.push_back(new_even_vertex);
        index = even_vertices.size() - 1;
      }

      as->faces[z].updated_vertices.push_back(index);
      /*even_vertices.push_back(new_even_vertex);
      index = even_vertices.size() - 1;*/
    }
  }

  //constructs new faces
  obj_vertices = even_vertices;
  obj_vertices.insert( obj_vertices.end(), odd_vertices.begin(), odd_vertices.end() );
  obj_faces.clear();
  for (auto face : as->faces) {
    int v1 = face.updated_vertices[0];
    int v2 = face.updated_vertices[1];
    int v3 = face.updated_vertices[2];

    int e1 = face.edge_vertices[0] + even_vertices.size();
    int e2 = face.edge_vertices[1] + even_vertices.size();
    int e3 = face.edge_vertices[2] + even_vertices.size();

    obj_faces.push_back(glm::uvec3(v1, e3, e2));
    obj_faces.push_back(glm::uvec3(v2, e1, e3));
    obj_faces.push_back(glm::uvec3(v3, e2, e1));
    obj_faces.push_back(glm::uvec3(e1, e2, e3));
    //std::cout << face.updated_vertices.size() << " " << face.edge_vertices.size() << '\n';
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
