#include "menger.h"
#include <iostream>
#include <cmath>

namespace {
	const int kMinLevel = 0;
	const int kMaxLevel = 4;
};

Menger::Menger()
{
	// Add additional initialization if you like
}

Menger::~Menger()
{
}

void
Menger::set_nesting_level(int level)
{
	nesting_level_ = level;
	dirty_ = true;
}

bool
Menger::is_dirty() const
{
	return dirty_;
}

void
Menger::set_clean()
{
	dirty_ = false;
}

void
Menger::set_dirty()
{
	dirty_ = true;
}

// FIXME generate Menger sponge geometry
void
Menger::generate_geometry(std::vector<glm::vec4>& obj_vertices,
                          std::vector<glm::uvec3>& obj_faces) const
{	
	//Start With One Cube
	obj_vertices.clear();
	obj_faces.clear();
	CreateCube(obj_vertices, obj_faces, 1.0f, glm::vec3(0.0f,0.0f, 0.0f));

	//For each nesting level generate cubes
	for(int L = 0; L < nesting_level_; L++){
		int vertice_size = obj_vertices.size();
		int faces_size = obj_faces.size();

		//Generate New Menger
		std::vector<glm::vec4> new_vertices;
		std::vector<glm::uvec3> new_faces;

		float cube_size = (float) 1.0f/std::pow(3,(L + 1));
		
		//For Cube
		for(int Cube = 0; Cube < vertice_size/8; Cube++ ){
			glm::vec4 parent_cube_pos = (obj_vertices[Cube*8] + obj_vertices[Cube*8 + 7]) / 2.0f;
			//printf("cube_size: %f, x: %f, y: %f, z: %f\n", cube_size, parent_cube_pos.x, parent_cube_pos.y, parent_cube_pos.z);
			
			//Subdivide into 27 cubes
			for(int i = 0; i < 27; i++){
				int x = (i % 3) - 1;
				int y = ((i/3) % 3) - 1;
				int z = ((i/9) % 3)  - 1;

				//Skip Inner Cubes
				if(std::abs(x) + std::abs(y) + std::abs(z) <= 1){
					continue;
				}
				
				glm::vec3 cube_pos = glm::vec3((float) x*cube_size + parent_cube_pos.x, (float) y*cube_size + parent_cube_pos.y,
												(float) z*cube_size + parent_cube_pos.z);

				//Do work
				CreateCube(new_vertices, new_faces, cube_size, cube_pos);
				//printf("cube_size: %f, x: %f, y: %f, z: %f\n", cube_size, cube_pos.x, cube_pos.y, cube_pos.z);
			}
		}

		//Clean Cube
		obj_vertices.clear();
		obj_faces.clear();

		//Replace Cube
		obj_vertices = new_vertices;
		obj_faces = new_faces;
	}

	// int vertice_size = obj_vertices.size();
	// int faces_size = obj_faces.size();

	// printf("nesting_level: %d, Vertices: %d, Faces: %d\n", nesting_level_, vertice_size, faces_size);
}

