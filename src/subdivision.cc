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
                          std::vector<glm::uvec3>& obj_faces, std::vector<glm::vec4>& sharp_crease_start,
                           std::vector<glm::vec4>& sharp_crease_end, std::vector<glm::vec4>& sticky_vertices)
{
	//std::cout << "test" << std::endl;
	as = new AdjacencyStructure(obj_vertices, obj_faces);
	//as->printStructure();
	std::vector<glm::vec4> odd_vertices;
  std::vector<glm::vec4> even_vertices;
  std::vector<glm::vec4> new_sharp_vertices = std::vector<glm::vec4>(obj_vertices.size());
  std::vector<glm::vec4> new_sharp_crease_start;
  std::vector<glm::vec4> new_sharp_crease_end;

  //computes new even vertices
  for (size_t z = 0; z < obj_faces.size(); z++) {
    /* code */
    for (size_t i = 0; i < 3; i++) {
      Vertex current = as->vertices[obj_faces[z][i]];
      int n = current.adj_vertices.size();
      float beta;
      glm::vec4 sum = glm::vec4(0.0, 0.0, 0.0, 0.0);
      glm::vec4 new_even_vertex;

      //checks if vertex is on sharp edge
      bool sharp = false;
      bool stuck = false;
      for (int j = 0; j < sharp_crease_start.size(); ++j)
      {
      	if (obj_vertices[obj_faces[z][i]] == sharp_crease_start[j])
      	{
      		sharp = true;
      	}
      }
      for (int j = 0; j < sharp_crease_end.size(); ++j)
      {
      	if (obj_vertices[obj_faces[z][i]] == sharp_crease_end[j])
      	{
      		sharp = true;
      	}
      }

      //checks if vertex is a sticky vertex
      for (int j = 0; j < sticky_vertices.size(); ++j)
      {
      	if (obj_vertices[obj_faces[z][i]] == sticky_vertices[j])
      	{
      		stuck = true;
      	}
      }

      if (stuck) {
      	new_even_vertex = obj_vertices[obj_faces[z][i]];
      }
      else if (sharp) {
      	int count = 0;
      	for (auto t : current.adj_vertices)
      	{
      		for (int j = 0; j < sharp_crease_start.size(); ++j)
      		{
      			if ((obj_vertices[obj_faces[z][i]] == sharp_crease_start[j] && obj_vertices[t] == sharp_crease_end[j]) ||
      				(obj_vertices[t] == sharp_crease_start[j] && obj_vertices[obj_faces[z][i]] == sharp_crease_end[j]))
      			{
      				sum = sum + obj_vertices[t];
      				count++;
      			}
      		}
      	}

      	if (count < 2) {
      		if (n == 3) {
	        	beta = 3.0 / 16.0;
	      	}
	      	else {
	        	beta = 3.0 / (8.0 * n);
	      	}

	      	for (auto v : current.adj_vertices) {
	        	sum = sum + obj_vertices[v];
	      	}

	      	new_even_vertex = (obj_vertices[obj_faces[z][i]] * (1 - (n * beta))) + sum * beta;
      	}
      	else if (count == 2) {
      		new_even_vertex = (0.75f) * obj_vertices[obj_faces[z][i]] + (0.125f) * sum;
      	}
      	else {
      		new_even_vertex = obj_vertices[obj_faces[z][i]];
      	}
      	//std::cout << count;

      	new_sharp_vertices[obj_faces[z][i]] = new_even_vertex;
      }
      else {
	      if (n == 3) {
	        beta = 3.0 / 16.0;
	      }
	      else {
	        beta = 3.0 / (8.0 * n);
	      }

	      for (auto v : current.adj_vertices) {
	        sum = sum + obj_vertices[v];
	      }

	      new_even_vertex = (obj_vertices[obj_faces[z][i]] * (1 - (n * beta))) + sum * beta;
	  }

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

  //computes odd vertices
	for (int x = 0; x < obj_faces.size(); x++) {
    glm::uvec3 face = obj_faces[x];
		std::vector<int> combination{0,1,2};
		for(auto i : combination){
			std::vector<int> current = combination;
			current.erase(current.begin() + i);
			glm::vec4 new_odd_vertix;

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

			bool sharp = false;
      		for (int j = 0; j < sharp_crease_start.size(); ++j)
      		{
      			if (obj_vertices[face[current[0]]] == sharp_crease_start[j])
      			{
      				if (obj_vertices[face[current[1]]] == sharp_crease_end[j])
      				sharp = true;
      			}

      			if (obj_vertices[face[current[0]]] == sharp_crease_end[j])
      			{
      				if (obj_vertices[face[current[1]]] == sharp_crease_start[j])
      				sharp = true;
      			}
      		}

      		if (sharp) {
      			new_odd_vertix = (0.5f)*(obj_vertices[face[current[0]]] + obj_vertices[face[current[1]]]);
      		}
      		else {
				new_odd_vertix = (0.375f)*(obj_vertices[face[current[0]]] + obj_vertices[face[current[1]]]) + (0.125f)*(obj_vertices[outer_v0] + obj_vertices[outer_v1]);
			}

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
      	if (sharp) {
      		new_sharp_crease_start.push_back(new_sharp_vertices[face[current[0]]]);
			new_sharp_crease_end.push_back(new_odd_vertix);
			new_sharp_crease_start.push_back(new_odd_vertix);
			new_sharp_crease_end.push_back(new_sharp_vertices[face[current[1]]]);
      	}
        odd_vertices.push_back(new_odd_vertix);
        index = odd_vertices.size() - 1;
      }

      as->faces[x].edge_vertices.push_back(index);
		}
	}

  //constructs new faces
  obj_vertices = even_vertices;
  obj_vertices.insert( obj_vertices.end(), odd_vertices.begin(), odd_vertices.end() );
  obj_faces.clear();

  sharp_crease_start = new_sharp_crease_start;
  sharp_crease_end = new_sharp_crease_end;

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
                          std::vector<glm::uvec4>& obj_faces,  std::vector<int>& sharp_vertices, std::vector<std::vector<int>>& creases)
{
	qas = new QuadAdjacencyStructure(obj_vertices, obj_faces);
	qas->printStructure();

	int total_fp = obj_faces.size();
	int total_ep = qas->edges.size();
	std::vector<glm::vec4> face_points = std::vector<glm::vec4>(total_fp);
	std::vector<glm::vec4> edge_points = std::vector<glm::vec4>(total_ep);
	std::vector<glm::vec4> control_points = std::vector<glm::vec4>(obj_vertices.size());

	//Calculate face points
	for(int i = 0; i < obj_faces.size(); i++){
		glm::vec4 point = obj_vertices[obj_faces[i].x] + obj_vertices[obj_faces[i].y] + obj_vertices[obj_faces[i].z] + obj_vertices[obj_faces[i].w];
		point = point / 4.0f;

		face_points[i] = point;
		
	}

	//Caluclate edge points
	for(int i = 0; i < qas->edges.size(); i++){
		if(creases[qas->edges[i].v0][qas->edges[i].v1]){
			glm::vec4 point = obj_vertices[qas->edges[i].v0] + obj_vertices[qas->edges[i].v1];
			point = point / 2.0f;

			edge_points[i] = point;
		} else{			//Smooth
			//Get Shared Faces
			int v0 = qas->edges[i].v0;
			int v1 = qas->edges[i].v1;

			qas->edges[i].v0;
			std::set<int> intersect;
			std::set_intersection(qas->vertices[v0].adj_faces.begin(), qas->vertices[v0].adj_faces.end(), qas->vertices[v1].adj_faces.begin(), qas->vertices[v1].adj_faces.end(), std::inserter(intersect, intersect.begin()));

			std::set<int> outer_vertices;

			for(auto face : intersect){
				outer_vertices.insert(obj_faces[face].x);
				outer_vertices.insert(obj_faces[face].y);
				outer_vertices.insert(obj_faces[face].z);
				outer_vertices.insert(obj_faces[face].w);
			}
			outer_vertices.erase(v0);
			outer_vertices.erase(v1);

			glm::vec4 point = obj_vertices[qas->edges[i].v0] + obj_vertices[qas->edges[i].v1];
			point = point * (0.375f);

			for(auto v : outer_vertices){
				point = point + (0.0625f)*obj_vertices[v];
			}

			edge_points[i] = point;
		}

	}

	//Calculate new control points
	for(int i = 0; i < obj_vertices.size(); i++){
		if(sharp_vertices[i] == 1){
			control_points[i] = obj_vertices[i];
			continue;
		}

		int sharp_edges = 0;
		std::vector<int> adj_sharp;
		for(int k = 0; k < obj_vertices.size(); k++){
			int v0 = (i < k) ? i : k;
			int v1 = (i < k) ? k : i;
			sharp_edges += creases[v0][v1];

			if(creases[v0][v1] == 1){
				for(auto edge : qas->edges){
					if(edge.v0 == v0 && edge.v1 == v1){
						adj_sharp.emplace_back(edge.id);
					}
				}	
			}

		}

		//std::cout << sharp_edges << std::endl;
		if(sharp_edges < 2){ //Smooth Rule
			float n = qas->vertices[i].adj_edges.size();
			float m = qas->vertices[i].adj_faces.size();
			glm::vec4 point = glm::vec4(0,0,0,0);

			for(auto face : qas->vertices[i].adj_faces){
				point = point + (face_points[face])/(n*m);
			}

			for(auto edge : qas->vertices[i].adj_edges){
				point = point + 2.0f*(edge_points[edge])/(n*n);
			}

			point = point + (obj_vertices[i]*(n-3.0f)/n);

			control_points[i] = point;

		} else if(sharp_edges == 2){//Crease Vertex Rule
			glm::vec4 point = (0.75f)*obj_vertices[i];
			for(auto edge : adj_sharp){
				point = point + (0.125f)*edge_points[edge];
			}
			control_points[i] = point;
		} else {//Corner Rule
			control_points[i] = obj_vertices[i];
		} 

		//std::cout << point.x << " " << point.y << " " << point.z << " " << point.w << std::endl;
	}

	//Connect Points
	std::vector<std::vector<int>> new_creases;
	std::vector<glm::vec4> new_vertices;
	std::vector<glm::uvec4> new_faces;

	new_vertices.insert( new_vertices.end(), face_points.begin(), face_points.end() );
	new_vertices.insert( new_vertices.end(), edge_points.begin(), edge_points.end() );
	new_vertices.insert( new_vertices.end(), control_points.begin(), control_points.end() );
	// for( auto point : new_vertices){
	// 	std::cout << point.x << " " << point.y << " " << point.z << " " << point.w << std::endl;
	// }

	new_creases = std::vector<std::vector<int>>(new_vertices.size());
  	for(int i = 0; i < new_creases.size(); i++){
  		new_creases[i] = std::vector<int>(new_vertices.size());
  	}

	std::vector<int>new_sharp_vertices = std::vector<int>(new_vertices.size());
	for(int i = 0; i < sharp_vertices.size(); i++){
		new_sharp_vertices[i + total_fp + total_ep] = sharp_vertices[i];
	}

	for(int i = 0; i < total_fp; i++){
		for(int j=0; j < 4; j++){ //per face vertex
			int vidx =  obj_faces[i][j];
			int adj0 =  obj_faces[i][(j + 1) % 4];
			int adj1 =  obj_faces[i][(j + 3) % 4];

			int edge_point0, edge_point1;
			//find first edge point
			for(auto edge : qas->vertices[vidx].adj_edges){
				int v0 = (vidx < adj0) ? vidx : adj0;
				int v1 = (vidx < adj0) ? adj0 : vidx;
				if(qas->edges[edge].v0 == v0 && qas->edges[edge].v1 == v1){
					edge_point0 = qas->edges[edge].id;
					if(creases[v0][v1]){
						new_creases[total_fp + edge_point0][total_fp + total_ep + vidx] = 1;
					}
					break;
				}
			}
			//find second edge point
			for(auto edge : qas->vertices[vidx].adj_edges){
				int v0 = (vidx < adj1) ? vidx : adj1;
				int v1 = (vidx < adj1) ? adj1 : vidx;
				if(qas->edges[edge].v0 == v0 && qas->edges[edge].v1 == v1){
					edge_point1 = qas->edges[edge].id;
					if(creases[v0][v1]){
						new_creases[total_fp + edge_point1][total_fp + total_ep + vidx] = 1;
					}
					break;
				}
			}

			//make new_face and add to faces
			glm::uvec4 new_face = glm::uvec4(0,0,0,0);
			new_face.x = total_fp + total_ep + vidx; //new control point
			new_face.y = total_fp + edge_point0; //first edge point
			new_face.z = i; //the face point
			new_face.w = total_fp + edge_point1;

			new_faces.emplace_back(new_face);
		}
	}

	obj_vertices.clear();
	obj_faces.clear();

	sharp_vertices = new_sharp_vertices;
	creases = new_creases;

	obj_vertices.insert( obj_vertices.end(), new_vertices.begin(), new_vertices.end() );
	obj_faces.insert( obj_faces.end(), new_faces.begin(), new_faces.end() );

	// for( auto crease : creases){
	// 	for( auto v : crease){
	// 		std::cout << v  << " ";
	// 	}
	// 	std::cout << std::endl;
	// }

	// for( auto point : obj_vertices){
	// 	std::cout << point.x << " " << point.y << " " << point.z << " " << point.w << std::endl;
	// }
	// for( auto face : obj_faces){
	// 	std::cout << face.x << " " << face.y << " " << face.z << " " << face.w << std::endl;
	// }




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
