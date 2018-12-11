#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <debuggl.h>
#include "menger.h"
#include "subdivision.h"
#include "camera.h"

int window_width = 800, window_height = 600;
double current_mouse_x, current_mouse_y;
bool fill = true;
bool wireframes = false, show_sphere = false;
bool ocean = false;
float t = 0.0f;
float wave_time = -100.0f;

int inner_level = 1, outer_level = 1;

int polycount = 3;
int sub = 0;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kOceanVao, kSphereVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

std::vector<glm::vec4> obj_vertices;
std::vector<glm::uvec3> obj_faces;

std::vector<int> sharp_crease_start_index;
std::vector<int> sharp_crease_end_index;
std::vector<int> sticky_vertices_indexes;

std::vector<glm::vec4> sharp_crease_start;
std::vector<glm::vec4> sharp_crease_end;
std::vector<glm::vec4> sticky_vertices;

std::vector<glm::vec4> load_obj_vertices;
std::vector<glm::uvec3> load_obj_faces;

std::vector<int> sharp_vertices;
std::vector<std::vector<int>> creases;
std::vector<glm::vec4> ocean_vertices;
std::vector<glm::uvec4> ocean_faces;

std::vector<int> sub_sharp_vertices;
std::vector<std::vector<int>> sub_creases;
std::vector<glm::vec4> original_ocean_vertices;
std::vector<glm::uvec4> original_ocean_faces;



// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal
const char* vertex_shader =
R"zzz(#version 330 core
in vec4 vertex_position;
uniform mat4 view;
uniform vec4 light_position;
out vec4 vs_light_direction;
out vec4 vs_world_position;
void main()
{
	vs_world_position = vertex_position;
	gl_Position = view * vertex_position;
	vs_light_direction = -gl_Position + view * light_position;
}
)zzz";

const char* tc_shader =
R"zzz(#version 410 core
layout (vertices = 3) out;
in vec4 vs_light_direction[];
in vec4 vs_world_position[];

out vec4 cp_light_direction[];
out vec4 cp_world_position[];

uniform int inner_level;
uniform int outer_level;

void main()
{
	// Set the control points of the output patch
	cp_light_direction[gl_InvocationID] = vs_light_direction[gl_InvocationID];
	cp_world_position[gl_InvocationID]  = vs_world_position[gl_InvocationID];

	//Set Levels
	gl_TessLevelOuter[0] = outer_level;
    gl_TessLevelOuter[1] = outer_level;
    gl_TessLevelOuter[2] = outer_level;
    gl_TessLevelInner[0] = inner_level;
}
)zzz";

const char* te_shader =
R"zzz(#version 410 core
layout(triangles, equal_spacing, ccw) in;
in vec4 cp_light_direction[];
in vec4 cp_world_position[];

out vec4 vs_light_direction;
out vec4 vs_world_position;

uniform mat4 view;


vec4 interpolate3D(vec4 v0, vec4 v1, vec4 v2)
{
    return vec4(gl_TessCoord.x) * v0 + vec4(gl_TessCoord.y) * v1 + vec4(gl_TessCoord.z) * v2;
}

void main()
{
	// interpolate
	vs_world_position =  interpolate3D(cp_world_position[0], cp_world_position[1], cp_world_position[2]);
	vs_light_direction = interpolate3D(cp_light_direction[0], cp_light_direction[1], cp_light_direction[2]);

	gl_Position = view * vs_world_position;
}
)zzz";

const char* geometry_shader =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
in vec4 vs_light_direction[];
in vec4 vs_world_position[];
flat out vec4 normal;
flat out vec4 global_normal;
out vec4 light_direction;
out vec4 world_position;
out vec3 bary_coord;
out float area;
void main()
{
	int n = 0;

	//Calculate World Normal
	global_normal = vec4(0.0, 0.0, 0.0, 0.0);
	vec3 A = vec3(vs_world_position[0]);
	vec3 B = vec3(vs_world_position[1]);
	vec3 C = vec3(vs_world_position[2]);
	vec3 cross_prod = cross(B-A,C-A);
	global_normal = vec4(normalize(cross_prod), 0.0);
	area = length(cross_prod);

	//Calculate Normal
	normal = vec4(0.0, 0.0, 0.0, 0.0);
	A = vec3(gl_in[0].gl_Position);
	B = vec3(gl_in[1].gl_Position);
	C = vec3(gl_in[2].gl_Position);
	normal = vec4(normalize(cross(B-A,C-A)), 0.0);

	for (n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		world_position = vs_world_position[n];
		gl_Position = projection * gl_in[n].gl_Position;
		bary_coord = vec3(0.0,0.0,0.0);
		bary_coord[n] = 1.0;

		EmitVertex();
	}

	EndPrimitive();
}
)zzz";

const char* fragment_shader =
R"zzz(#version 330 core
flat in vec4 normal;
flat in vec4 global_normal;
in vec4 light_direction;
out vec4 fragment_color;
void main()
{
	vec4 color = vec4(1.0, 0.0, 0.0, 1.0);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);

	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz";

// FIXME: Implement shader effects with an alternative shader.
const char* floor_fragment_shader =
R"zzz(#version 330 core
uniform bool show_wireframes;
flat in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
in vec3 bary_coord;
in float area;
out vec4 fragment_color;

void main()
{
	fragment_color = vec4(0.0, 0.0, 0.0, 1.0);
	if (mod(floor(world_position.x) + floor(world_position.z), 2) == 1) {
		fragment_color = vec4(1.0, 1.0, 1.0, 1.0);
	}

	if(show_wireframes){
		float min_value = min(min(bary_coord[0], bary_coord[1]), bary_coord[2]);
		if ( (min_value * sqrt(area)) <= 0.025){
			fragment_color = vec4(0.0f,1.0f,0.0f,1.0f); //Green Just Like This Text
		}
	}

	//fragment_color = vec4(bary_coord, 1.0f);
	float dot_nl = -dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.01, 1.0);
	fragment_color = clamp(dot_nl * fragment_color, 0.0, 1.0);
}
)zzz";

// FIXME: Implement shader effects with an alternative shader.
const char* sphere_fragment_shader =
R"zzz(#version 330 core
uniform bool show_wireframes;
flat in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
in vec3 bary_coord;
in float area;
out vec4 fragment_color;

void main()
{
	fragment_color = vec4(0.8, 1.0, 0.5, 1.0);

	if(show_wireframes){
		float min_value = min(min(bary_coord[0], bary_coord[1]), bary_coord[2]);
		if ( (min_value * sqrt(area)) <= 0.025){
			fragment_color = vec4(0.0f,1.0f,0.0f,1.0f); //Green Just Like This Text
		}
	}

	//fragment_color = vec4(bary_coord, 1.0f);
	float dot_nl = -dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.01, 1.0);
	fragment_color = clamp(dot_nl * fragment_color, 0.0, 1.0);
}
)zzz";


const char* ocean_fragment_shader =
R"zzz(#version 330 core
uniform bool show_wireframes;
uniform vec3 eye_position;

in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
in vec3 bary_coord;
in float area;
out vec4 fragment_color;

void main()
{
	fragment_color = vec4(0.6, 0.8, 1.0, 1.0);

	//fragment_color = vec4(bary_coord, 1.0f);
	float dot_nl = -dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.00, 1);

	vec3 r = normalize(eye_position - vec3(world_position));
	vec3 ln = (dot(vec3(normal), vec3(light_direction)) / dot(vec3(normal), vec3(normal))) * vec3(normal);
	vec3 reflected_l = normalize(ln + ln - vec3(light_direction));
	float dot_lr = clamp(dot(reflected_l, r),0.0f,1.0f);

	vec4 A = 0.20f * fragment_color; //Ambient
	vec4 D = dot_nl * fragment_color * 0.85f; //Diffusive
	fragment_color = clamp(A + D, 0.0, 1.0);

	if(show_wireframes){
		float min_value = min(min(bary_coord[0], bary_coord[1]), bary_coord[2]);
		if ( (min_value * sqrt(area)) <= 0.025){
			fragment_color = vec4(0.0f,1.0f,0.0f,1.0f); //Green Just Like This Text
		}
	}


}
)zzz";

const char* ocean_tc_shader =
R"zzz(#version 410 core
layout (vertices = 4) out;
in vec4 vs_light_direction[];
in vec4 vs_world_position[];

out vec4 cp_light_direction[];
out vec4 cp_world_position[];

uniform int inner_level;
uniform int outer_level;

uniform float wave_x;

void main()
{
	// Set the control points of the output patch
	cp_light_direction[gl_InvocationID] = vs_light_direction[gl_InvocationID];
	cp_world_position[gl_InvocationID]  = vs_world_position[gl_InvocationID];

	//float dist = distance(vec2(wave_x, 0), vec2(vs_world_position[gl_InvocationID][0], vs_world_position[gl_InvocationID][2]));
	//float mult = clamp(4 - dist, 1, 4);

	if(gl_InvocationID == 0){
		//Set Levels
		gl_TessLevelOuter[0] = outer_level;// * mult;
		gl_TessLevelOuter[1] = outer_level;// * mult;
		gl_TessLevelOuter[2] = outer_level;// * mult;
		gl_TessLevelOuter[3] = outer_level;// * mult;
		gl_TessLevelInner[0] = inner_level;// * mult;
		gl_TessLevelInner[1] = inner_level;// * mult;
	}
}
)zzz";

const char* ocean_te_shader =
R"zzz(#version 410 core
layout(quads) in;
in vec4 cp_light_direction[];
in vec4 cp_world_position[];

out vec4 vs_light_direction;
out vec4 vs_world_position;

uniform vec4 light_position;
uniform mat4 view;

uniform float wave_x;
uniform float t;

vec4 interpolate3D(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
	vec3 p0 = mix(vec3(v0), vec3(v3), gl_TessCoord.x);
	vec3 p1 = mix(vec3(v1), vec3(v2), gl_TessCoord.x);

	vec3 p =  mix(p0, p1, gl_TessCoord.y);

	return vec4(p, 1.0);
}

void main()
{
	// interpolate
	vs_world_position =  interpolate3D(cp_world_position[0], cp_world_position[1], cp_world_position[2], cp_world_position[3]);
	
	gl_Position = view * vs_world_position;
	vs_light_direction = -gl_Position + view * light_position;

}
)zzz";

const char* ocean_geometry_shader =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
in vec4 vs_light_direction[];
in vec4 vs_world_position[];
in vec4 wave_normal[];

out vec4 normal;
out vec4 light_direction;
out vec4 world_position;
out vec3 bary_coord;
out float area;
void main()
{
	int n = 0;

	//Calculate Area
	vec3 A = vec3(vs_world_position[0]);
	vec3 B = vec3(vs_world_position[1]);
	vec3 C = vec3(vs_world_position[2]);
	vec3 cross_prod = cross(B - A,C-A);
	area = length(cross_prod);

	normal = vec4(normalize(cross_prod), 0.0);

	for (n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		world_position = vs_world_position[n];
		gl_Position = projection * gl_in[n].gl_Position;
		bary_coord = vec3(0.0,0.0,0.0);
		bary_coord[n] = 1.0;
		EmitVertex();
	}

	EndPrimitive();
}
)zzz";

void
CreateTriangle(std::vector<glm::vec4>& vertices,
        std::vector<glm::uvec3>& indices)
{
	vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));
	indices.push_back(glm::uvec3(0, 1, 2));
}

void
CreateFloor(std::vector<glm::vec4>& vertices,
        std::vector<glm::uvec3>& indices)
{
	//vertices.push_back(glm::vec4(0.0f, -2.0f, 0.0f, 1.0f));
	vertices.push_back(glm::vec4(10.0f, -2.0f, -10.0f, 1.0f));
	vertices.push_back(glm::vec4(10.0f, -2.0f, 10.0f, 1.0f));
	vertices.push_back(glm::vec4(-10.0f, -2.0f, 10.0f,1.0f));
	vertices.push_back(glm::vec4(-10.0f, -2.0f, -10.0f, 1.0f));
	indices.push_back(glm::uvec3(0, 1, 2));
	indices.push_back(glm::uvec3(0, 2, 3));
	//indices.push_back(glm::uvec3(0, 3, 4));
	//indices.push_back(glm::uvec3(0, 4, 1));
}

void
CreateOcean(std::vector<glm::vec4>& vertices,
			std::vector<glm::uvec4>& indices, float t){

	vertices.clear();
	indices.clear();


	double step = (40.0f)/32.0f;

	for(int i = 0; i < 16; i++){
		for(int j = 0; j < 16; j++){
			float x = i*2*step - 20.0f + step;
			float z = j*2*step - 20.0f + step;
			int index = (i*16 + j)*4;

			vertices.push_back(glm::vec4(x - step, -2.0f, z - step, 1.0f));
			vertices.push_back(glm::vec4(x - step, -2.0f, z + step,1.0f));
			vertices.push_back(glm::vec4(x + step, -2.0f, z + step, 1.0f));
			vertices.push_back(glm::vec4(x + step, -2.0f, z - step, 1.0f));
			indices.push_back(glm::uvec4(index, index + 1, index + 2, index + 3));
		}
	}


}

void
CreateSphere(std::vector<glm::vec4>& vertices,
			std::vector<glm::uvec3>& indices){

	const float t = (1.0 + std::sqrt(5.0)) / 2.0;

	// Vertices
	vertices.push_back(glm::normalize(glm::vec4(-1.0,  t, 0.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4( 1.0,  t, 0.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(-1.0, -t, 0.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4( 1.0, -t, 0.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(0.0, -1.0,  t, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(0.0,  1.0,  t, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(0.0, -1.0, -t, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(0.0,  1.0, -t, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(t, 0.0, -1.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(t, 0.0,  1.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(-t, 0.0, -1.0, 0.0)));
	vertices.push_back(glm::normalize(glm::vec4(-t, 0.0,  1.0, 0.0)));

	// Faces
	indices.push_back(glm::uvec3(0, 11, 5));
	indices.push_back(glm::uvec3(0, 5, 1));
	indices.push_back(glm::uvec3(0, 1, 7));
	indices.push_back(glm::uvec3(0, 7, 10));
	indices.push_back(glm::uvec3(0, 10, 11));
	indices.push_back(glm::uvec3(1, 5, 9));
	indices.push_back(glm::uvec3(5, 11, 4));
	indices.push_back(glm::uvec3(11, 10, 2));
	indices.push_back(glm::uvec3(10, 7, 6));
	indices.push_back(glm::uvec3(7, 1, 8));
	indices.push_back(glm::uvec3(3, 9, 4));
	indices.push_back(glm::uvec3(3, 4, 2));
	indices.push_back(glm::uvec3(3, 2, 6));
	indices.push_back(glm::uvec3(3, 6, 8));
	indices.push_back(glm::uvec3(3, 8, 9));
	indices.push_back(glm::uvec3(4, 9, 5));
	indices.push_back(glm::uvec3(2, 4, 11));
	indices.push_back(glm::uvec3(6, 2, 10));
	indices.push_back(glm::uvec3(8, 6, 7));
	indices.push_back(glm::uvec3(9, 8, 1));


	for(int j = 0; j < 4; j++){
		int i_size = indices.size();

		for(int i = 0; i < i_size; i++){
			int v_size = vertices.size();

			glm::vec4 v0 = vertices[indices[i][0]];
			glm::vec4 v1 = vertices[indices[i][1]];
			glm::vec4 v2 = vertices[indices[i][2]];
			glm::vec4 v3 = glm::normalize(0.5f * (v0 + v1));
			glm::vec4 v4 = glm::normalize(0.5f * (v1 + v2));
			glm::vec4 v5 = glm::normalize(0.5f * (v2 + v0));
			vertices.push_back(v0);
			vertices.push_back(v1);
			vertices.push_back(v2);
			vertices.push_back(v3);
			vertices.push_back(v4);
			vertices.push_back(v5);

			indices.push_back(glm::uvec3(v_size + 0, v_size + 3, v_size + 5));
			indices.push_back(glm::uvec3(v_size + 3, v_size + 1, v_size + 4));
			indices.push_back(glm::uvec3(v_size + 5, v_size + 4, v_size + 2));
			indices.push_back(glm::uvec3(v_size + 3, v_size + 4, v_size + 5));
		}

	}

	//printf("%f, %f, %f\n", vertices[0][0], vertices[0][1], vertices[0][2]);

	for(int i = 0; i < vertices.size(); i++){
		//printf("%f, %f, %f\n", vertices[i][0], vertices[i][1], vertices[i][2]);
		vertices[i][0] -= 10.0f;
		vertices[i][1] += 10.0f;
		vertices[i][3] = 1.0f;

	}


}


// FIXME: Save geometry to OBJ file
void
SaveObj()
{
  std::ofstream myfile ("geometry.obj");
  if (myfile.is_open())
  {
	for(int i = 0; i < obj_vertices.size(); i++){
       myfile << "v " << obj_vertices[i][0] << " " << obj_vertices[i][1] <<  " " << obj_vertices[i][2] << std::endl;
	}

	for(int i = 0; i < obj_faces.size(); i++){
       myfile << "f " << obj_faces[i][0] << " " << obj_faces[i][1] <<  " " << obj_faces[i][2] << std::endl;
	}
    myfile.close();
  }
}

void LoadQuadObj(std::string filename, std::vector<glm::vec4>& vertices,
			std::vector<glm::uvec4>& indices){
	vertices.clear();
	indices.clear();
	sharp_vertices.clear();
	creases.clear();

	polycount = 4;
	glPatchParameteri(GL_PATCH_VERTICES, polycount);
	std::ifstream myfile (filename);

	std::vector<std::pair<int,int>> sharp_edges;
	if (myfile.is_open())
  	{
  		std::string line;

		while (! myfile.eof() )
    	{

      		getline (myfile,line);


      		//Add Creases
      		if(line.c_str()[0] == 'e'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}
    			sharp_edges.emplace_back(std::pair<int,int>(stoi(tokens[1]) - 1,stoi(tokens[2]) - 1));

      		}
      		//add corners
      		if(line.c_str()[0] == 'c'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}

    			std::cout << "c ";
    			for(auto& v : tokens){
    				if(v == "c"){continue;}
    				int p = stoi(v);
    				std::cout << p << " ";
    				sharp_vertices.push_back(p - 1);
    			}
    			std::cout << std::endl;
      		}

      		//Add New Vertex
      		if(line.c_str()[0] == 'v'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}

    			std::cout << "v ";
    			std::vector<float> new_vertex;
    			for(auto& v : tokens){
    				if(v == "v"){continue;}
    				float p = stof(v)/1.0f;
    				std::cout << p << " ";
    				new_vertex.push_back(p);
    			}

    			vertices.push_back(glm::vec4(new_vertex[0], new_vertex[1], new_vertex[2], 1.0f));
    			std::cout << std::endl;
      		}

      		//Add New Face
      		if(line.c_str()[0] == 'f'){
      			
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			std::string value;

      			while(getline(stream, x, ' '))
    			{

    				std::stringstream valuestream(x);

    				getline(valuestream, value, '/');
        			tokens.push_back(value);
    			}

    			std::cout << "f ";
    			std::vector<float> new_face;
    			int i = 0;
    			for(auto& v : tokens){
    				i++;
    				if(v == "f"){continue;}
    				if(v == ""){continue;}
    				if(i > 5){continue;}

    				//std::cout << v.size();
    				
    				int p = stoi(v);
    				std::cout << p << " ";
    				new_face.push_back(p - 1);
    			}

    			indices.push_back(glm::uvec4(new_face[0], new_face[1], new_face[2], new_face[3]));
    			std::cout << std::endl;
      		}
      	}
  	}

  	std::vector<int> new_sharp_vertices = std::vector<int>(vertices.size());
  	for(auto sv : sharp_vertices){
  		new_sharp_vertices[sv] = 1;
  	}
	
  	sharp_vertices = new_sharp_vertices;

  	// std::cout << "-------\n";
  	// for(auto sv : sharp_vertices){
  	// 	std::cout << sv << std::endl;
  	// }

  	creases = std::vector<std::vector<int>>(vertices.size());
  	for(int i = 0; i < creases.size(); i++){
  		creases[i] = std::vector<int>(vertices.size());
  	}

  	for(auto se : sharp_edges){
  		std::cout << se.first << " " << se.second << std::endl;
  		int v0 = (se.first < se.second) ? se.first : se.second;
  		int v1 = (se.first < se.second) ? se.second : se.first;
  		creases[v0][v1] = 1;
  	}

  	myfile.close();
	std::cout << vertices.size() << std::endl;
	std::cout << indices.size() << std::endl;
}

void LoadObj(std::string filename, std::vector<glm::vec4>& vertices,
			std::vector<glm::uvec3>& indices,std::vector<int>& sharp_crease_start_index, 
			std::vector<int>& sharp_crease_end_index, std::vector<int>& sticky_vertices_indexes)
{
<<<<<<< HEAD
	polycount = 3;
	glPatchParameteri(GL_PATCH_VERTICES, polycount);
=======
>>>>>>> 1bae9352c2e56ec2b9d4d42cbd10ede74766da02
	vertices.clear();
	indices.clear();

	std::ifstream myfile (filename);

	if (myfile.is_open())
  	{
  		std::string line;
		while (! myfile.eof() )
    	{
      		getline (myfile,line);

      		//Add New Vertex
      		if(line.c_str()[0] == 'v'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}

    			std::cout << "v ";
    			std::vector<float> new_vertex;
    			for(auto& v : tokens){
    				if(v == "v"){continue;}
    				float p = stof(v);
    				std::cout << p << " ";
    				new_vertex.push_back(p);
    			}

    			vertices.push_back(glm::vec4(new_vertex[0], new_vertex[1], new_vertex[2], 1.0f));
    			std::cout << std::endl;
      		}

      		//Add New Face
      		if(line.c_str()[0] == 'f'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}

    			std::cout << "f ";
    			std::vector<float> new_face;
    			for(auto& v : tokens){
    				if(v == "f"){continue;}
    				int p = stoi(v);
    				std::cout << p << " ";
    				new_face.push_back(p - 1);
    			}

    			indices.push_back(glm::uvec3(new_face[0], new_face[1], new_face[2]));
    			std::cout << std::endl;
      		}

      		//adds sharp creases
      		if(line.c_str()[0] == 's' && line.c_str()[1] == 'e'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}

    			std::cout << "se ";
    			std::vector<float> new_crease;
    			for(auto& v : tokens){
    				if(v == "se"){continue;}
    				int p = stoi(v);
    				std::cout << p << " ";
    				new_crease.push_back(p - 1);
    			}

    			sharp_crease_start_index.push_back(new_crease[0]);
    			sharp_crease_end_index.push_back(new_crease[1]);
    			std::cout << std::endl;
      		}

      		//adds sticky vertices
      		if(line.c_str()[0] == 's' && line.c_str()[1] == 'v'){
      			std::stringstream stream(line);
      			std::vector<std::string> tokens;
      			std::string x;
      			while(getline(stream, x, ' '))
    			{
        			tokens.push_back(x);
    			}

    			std::cout << "sv ";
    			float sticky_vertex;
    			for(auto& v : tokens){
    				if(v == "sv"){continue;}
    				int p = stoi(v);
    				std::cout << p << " ";
    				sticky_vertex = p - 1;
    			}

    			sticky_vertices_indexes.push_back(sticky_vertex);
    			std::cout << std::endl;
      		}

      	}
  	}

  	myfile.close();
	std::cout << vertices.size() << std::endl;
	std::cout << indices.size() << std::endl;

}

void
ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<Menger> g_menger;
std::shared_ptr<Subdivision> g_sub;
Camera g_camera;

void
KeyCallback(GLFWwindow* window,
            int key,
            int scancode,
            int action,
            int mods)
{
	// Note:
	// This is only a list of functions to implement.
	// you may want to re-organize this piece of code.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		SaveObj();
	} else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
		// FIXME: WASD
		g_camera.forward(1);
	} else if (key == GLFW_KEY_S && action != GLFW_RELEASE && mods != GLFW_MOD_CONTROL) {
		g_camera.forward(-1);
	} else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
		g_camera.strafe(-1);
	} else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
		g_camera.strafe(1);
	} else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
		// FIXME: Left Right Up and Down
		g_camera.roll(-1);
	} else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
		g_camera.roll(1);
	} else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
		g_camera.up(-1);
	} else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
		g_camera.up(1);
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		// FIXME: FPS mode on/off
	}
	if (!g_menger)
		return ; // 0-4 only available in Menger mode.
	if (key == GLFW_KEY_0 && action != GLFW_RELEASE) {
		// FIXME: Change nesting level of g_menger
		g_menger->set_nesting_level(0);
		// Note: GLFW_KEY_0 - 4 may not be continuous.
	} else if (key == GLFW_KEY_1 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(1);
	} else if (key == GLFW_KEY_2 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(2);
	} else if (key == GLFW_KEY_3 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(3);
	} else if (key == GLFW_KEY_4 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(4);
	} else if (key == GLFW_KEY_F && action != GLFW_RELEASE && mods == GLFW_MOD_CONTROL){
		fill = !fill;
		if(fill){
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		} else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
	} else if (key == GLFW_KEY_F && action != GLFW_RELEASE && mods != GLFW_MOD_CONTROL){
		wireframes = !wireframes;
	} else if (key == GLFW_KEY_MINUS && action != GLFW_RELEASE){
		//outer_level = (outer_level - 1) ? outer_level - 1 : 1;
		//printf("new outer level: %d\n", outer_level);
		sub = (sub == 0) ? 0 : sub - 1;
		sub_creases = creases;
		sub_sharp_vertices = sharp_vertices;
		ocean_vertices = original_ocean_vertices;
		ocean_faces = original_ocean_faces;

		for(int i = 0; i < sub; i++){
			g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sub_sharp_vertices, sub_creases);
		}

	} else if (key == GLFW_KEY_EQUAL && action != GLFW_RELEASE){
		sub = (sub > 9) ? sub : sub + 1;
		sub_creases = creases;
		sub_sharp_vertices = sharp_vertices;
		ocean_vertices = original_ocean_vertices;
		ocean_faces = original_ocean_faces;

		for(int i = 0; i < sub; i++){
			g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sub_sharp_vertices, sub_creases);
		}
		//outer_level = outer_level + 1 != 65 ? outer_level + 1 : 64;
		//printf("new outer level: %d\n", outer_level);
	} else if (key == GLFW_KEY_COMMA && action != GLFW_RELEASE){
		inner_level = (inner_level - 1) ? inner_level - 1 : 1;
		printf("new inner level: %d\n", inner_level);
	} else if (key == GLFW_KEY_PERIOD && action != GLFW_RELEASE){
		inner_level = inner_level + 1 != 65 ? inner_level + 1 : 64;
		printf("new inner level: %d\n", inner_level);
	} else if (key == GLFW_KEY_O && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		ocean = !ocean;
		int v = 3 + ocean;
		glPatchParameteri(GL_PATCH_VERTICES, v);
	} else if (key == GLFW_KEY_T && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		wave_time = t;
		//printf("Hello World!\n");
	} else if (key == GLFW_KEY_M && action == GLFW_RELEASE) {
		show_sphere = !show_sphere;
		//printf("Hello World!\n");
	}


}

int g_current_button;
bool g_mouse_pressed;

void
MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{

	if (!g_mouse_pressed){
	    current_mouse_x = mouse_x;
        current_mouse_y = mouse_y;
		return;
	}
	if (g_current_button == GLFW_MOUSE_BUTTON_LEFT) {
		//printf("x: %f, y: %f\n", mouse_x - current_mouse_x, mouse_y - current_mouse_y  );
		g_camera.rotate(mouse_x - current_mouse_x, mouse_y - current_mouse_y);
	} else if (g_current_button == GLFW_MOUSE_BUTTON_RIGHT) {
		// FIXME: middle drag
	} else if (g_current_button == GLFW_MOUSE_BUTTON_MIDDLE) {
		// FIXME: right drag
	}
	current_mouse_x = mouse_x;
    current_mouse_y = mouse_y;
}

void
MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	g_mouse_pressed = (action == GLFW_PRESS);
	g_current_button = button;
}

int main(int argc, char* argv[])
{
	std::string window_title = "Menger";
	//glfwInit();
	if (!glfwInit()){
		exit(EXIT_FAILURE);
	}
	char* obj_file;
	if(argc != 2){
		exit(EXIT_FAILURE);
	} else {
		obj_file = argv[1];
	}

	g_menger = std::make_shared<Menger>();
	g_sub = std::make_shared<Subdivision>();
	glfwSetErrorCallback(ErrorCallback);

	// Ask an OpenGL 4.1 core profile context
	// It is required on OSX and non-NVIDIA Linux
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(window_width, window_height,
			&window_title[0], nullptr, nullptr);
	CHECK_SUCCESS(window != nullptr);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MousePosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

        //FIXME: Create the geometry from a Menger object.


	g_menger->set_nesting_level(1);
	g_menger->generate_geometry(obj_vertices, obj_faces);
	g_menger->set_clean();


	LoadObj("../objects/cube_cylinder_sharp.s.obj", obj_vertices, obj_faces, sharp_crease_start_index, sharp_crease_end_index, sticky_vertices_indexes);
	for (int i = 0; i < sharp_crease_start_index.size(); ++i)
	{
		sharp_crease_start.push_back(obj_vertices[sharp_crease_start_index[i]]);
		sharp_crease_end.push_back(obj_vertices[sharp_crease_end_index[i]]);
	}

	for (int i = 0; i < sticky_vertices_indexes.size(); ++i)
	{
		sticky_vertices.push_back(obj_vertices[sticky_vertices_indexes[i]]);
	}

	//sets left side of cube.s.obj as sharp
	/*sharp_crease_start.push_back(obj_vertices[4]);
	sharp_crease_start.push_back(obj_vertices[12]);
	sharp_crease_start.push_back(obj_vertices[11]);
	sharp_crease_start.push_back(obj_vertices[7]);
	sharp_crease_end.push_back(obj_vertices[12]);
	sharp_crease_end.push_back(obj_vertices[11]);
	sharp_crease_end.push_back(obj_vertices[7]);
	sharp_crease_end.push_back(obj_vertices[4]);*/

	g_sub->loop_subdivision(obj_vertices, obj_faces, sharp_crease_start, sharp_crease_end, sticky_vertices);
	g_sub->loop_subdivision(obj_vertices, obj_faces, sharp_crease_start, sharp_crease_end, sticky_vertices);
	g_sub->loop_subdivision(obj_vertices, obj_faces, sharp_crease_start, sharp_crease_end, sticky_vertices);
	g_sub->loop_subdivision(obj_vertices, obj_faces, sharp_crease_start, sharp_crease_end, sticky_vertices);
	g_sub->loop_subdivision(obj_vertices, obj_faces, sharp_crease_start, sharp_crease_end, sticky_vertices);

	LoadQuadObj(obj_file, ocean_vertices, ocean_faces);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices, creases);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices, creases);
	original_ocean_faces = ocean_faces;
	original_ocean_vertices = ocean_vertices;
	// g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices, creases);
	// g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices, creases);


	std::cout << "-------\n";
  	for(auto sv : sharp_vertices){
  		std::cout << sv << std::endl;
  	}
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces, sharp_vertices);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces);
	//g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces);
    //g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces);
    //g_sub->catmull_clark_subdivision(ocean_vertices, ocean_faces);

	glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
	for (const auto& vert : obj_vertices) {
		min_bounds = glm::min(vert, min_bounds);
		max_bounds = glm::max(vert, max_bounds);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";

	// Setup our VAO array.
	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

	// Switch to the VAO for Geometry.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * obj_faces.size() * 3,
				obj_faces.data(), GL_STATIC_DRAW));

	/*
 	 * By far, the geometry is loaded into g_buffer_objects[kGeometryVao][*].
	 * These buffers are binded to g_array_objects[kGeometryVao]
	 */

	// FIXME: load the floor into g_buffer_objects[kFloorVao][*],
	//        and bind these VBO to g_array_objects[kFloorVao]

	std::vector<glm::vec4> floor_vertices;
  	std::vector<glm::uvec3> floor_faces;
	CreateFloor(floor_vertices, floor_faces);
	//CreateTriangle(floor_vertices, floor_faces);

	// Switch to the VAO for Floor.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));;

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * floor_vertices.size() * 4, floor_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * floor_faces.size() * 3,
				floor_faces.data(), GL_STATIC_DRAW));

	//Create Sphere
	std::vector<glm::vec4> sphere_vertices;
  	std::vector<glm::uvec3> sphere_faces;
	CreateSphere(sphere_vertices, sphere_faces);
	//CreateTriangle(floor_vertices, floor_faces);

	// Switch to the VAO for Floor.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kSphereVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kSphereVao][0]));;

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kSphereVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * sphere_vertices.size() * 4, sphere_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kSphereVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * sphere_faces.size() * 3,
				sphere_faces.data(), GL_STATIC_DRAW));

	// FIXME: load the OCEAN into g_buffer_objects[kOceanVao][*],
	//        and bind these VBO to g_array_objects[kOceanVao]
	
	//CreateTriangle(floor_vertices, floor_faces);

	// Switch to the VAO for Floor.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kOceanVao][0]));;

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * ocean_vertices.size() * 4, ocean_vertices.data(),
				GL_DYNAMIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * ocean_faces.size() * 4,
				ocean_faces.data(), GL_DYNAMIC_DRAW));



	// Setup vertex shader.
	GLuint vertex_shader_id = 0;
	const char* vertex_source_pointer = vertex_shader;
	CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);

	// Setup geometry shader.
	GLuint geometry_shader_id = 0;
	const char* geometry_source_pointer = geometry_shader;
	CHECK_GL_ERROR(geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
	CHECK_GL_ERROR(glShaderSource(geometry_shader_id, 1, &geometry_source_pointer, nullptr));
	glCompileShader(geometry_shader_id);
	CHECK_GL_SHADER_ERROR(geometry_shader_id);

	// Setup fragment shader.
	GLuint fragment_shader_id = 0;
	const char* fragment_source_pointer = fragment_shader;
	CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, nullptr));
	glCompileShader(fragment_shader_id);
	CHECK_GL_SHADER_ERROR(fragment_shader_id);

	// Let's create our program.
	GLuint program_id = 0;
	CHECK_GL_ERROR(program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, fragment_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, geometry_shader_id));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "fragment_color"));
	glLinkProgram(program_id);
	CHECK_GL_PROGRAM_ERROR(program_id);

	// Get the uniform locations.
	GLint projection_matrix_location = 0;
	CHECK_GL_ERROR(projection_matrix_location =
			glGetUniformLocation(program_id, "projection"));
	GLint view_matrix_location = 0;
	CHECK_GL_ERROR(view_matrix_location =
			glGetUniformLocation(program_id, "view"));
	GLint light_position_location = 0;
	CHECK_GL_ERROR(light_position_location =
			glGetUniformLocation(program_id, "light_position"));

	// Setup fragment shader for the floor
	GLuint floor_fragment_shader_id = 0;
	const char* floor_fragment_source_pointer = floor_fragment_shader;
	CHECK_GL_ERROR(floor_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_fragment_shader_id, 1,
				&floor_fragment_source_pointer, nullptr));
	glCompileShader(floor_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(floor_fragment_shader_id);

	// Setup tc_shader for the floor
	GLuint floor_tc_shader_id = 0;
	const char* floor_tc_source_pointer = tc_shader;
	CHECK_GL_ERROR(floor_tc_shader_id = glCreateShader(GL_TESS_CONTROL_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_tc_shader_id, 1,
				&floor_tc_source_pointer, nullptr));
	glCompileShader(floor_tc_shader_id);
	CHECK_GL_SHADER_ERROR(floor_tc_shader_id);

	// Setup te_shader for the floor
	GLuint floor_te_shader_id = 0;
	const char* floor_te_source_pointer = te_shader;
	CHECK_GL_ERROR(floor_te_shader_id = glCreateShader(GL_TESS_EVALUATION_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_te_shader_id, 1,
				&floor_te_source_pointer, nullptr));
	glCompileShader(floor_te_shader_id);
	CHECK_GL_SHADER_ERROR(floor_te_shader_id);

	// Setup fragment shader for the sphere
	GLuint sphere_fragment_shader_id = 0;
	const char* sphere_fragment_source_pointer = sphere_fragment_shader;
	CHECK_GL_ERROR(sphere_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(sphere_fragment_shader_id, 1,
				&sphere_fragment_source_pointer, nullptr));
	glCompileShader(sphere_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(sphere_fragment_shader_id);


	// FIXME: Setup another program for the floor, and get its locations.
	// Note: you can reuse the vertex and geometry shader objects

		// Let's create our program.
	GLuint floor_program_id = 0;
	CHECK_GL_ERROR(floor_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(floor_program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_fragment_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, geometry_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_tc_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_te_shader_id));

	// Bind attributes.
	//CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	CHECK_GL_ERROR(glBindAttribLocation(floor_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(floor_program_id, 0, "fragment_color"));
	glLinkProgram(floor_program_id);
	CHECK_GL_PROGRAM_ERROR(floor_program_id);

	// GLint MaxPatchVertices = 0;
	// glGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
	// printf("Max supported patch vertices %d\n", MaxPatchVertices);
	// glPatchParameteri(GL_PATCH_VERTICES, 3);

	// Get the uniform locations.
	GLint floor_projection_matrix_location = 0;
	CHECK_GL_ERROR(floor_projection_matrix_location =
			glGetUniformLocation(floor_program_id, "projection"));
	GLint floor_view_matrix_location = 0;
	CHECK_GL_ERROR(floor_view_matrix_location =
			glGetUniformLocation(floor_program_id, "view"));
	GLint floor_light_position_location = 0;
	CHECK_GL_ERROR(floor_light_position_location =
			glGetUniformLocation(floor_program_id, "light_position"));

	GLint wireframes_location = 0;
	CHECK_GL_ERROR(wireframes_location =
			glGetUniformLocation(floor_program_id, "show_wireframes"));
	GLint inner_level_location = 0;
	CHECK_GL_ERROR(inner_level_location =
			glGetUniformLocation(floor_program_id, "inner_level"));
	GLint outer_level_location = 0;
	CHECK_GL_ERROR(outer_level_location =
			glGetUniformLocation(floor_program_id, "outer_level"));



	// Setup tc_shader for the floor
	GLuint ocean_tc_shader_id = 0;
	const char* ocean_tc_source_pointer = ocean_tc_shader;
	CHECK_GL_ERROR(ocean_tc_shader_id = glCreateShader(GL_TESS_CONTROL_SHADER));
	CHECK_GL_ERROR(glShaderSource(ocean_tc_shader_id, 1,
				&ocean_tc_source_pointer, nullptr));
	glCompileShader(ocean_tc_shader_id);
	CHECK_GL_SHADER_ERROR(ocean_tc_shader_id);

	// Setup te_shader for the floor
	GLuint ocean_te_shader_id = 0;
	const char* ocean_te_source_pointer = ocean_te_shader;
	CHECK_GL_ERROR(ocean_te_shader_id = glCreateShader(GL_TESS_EVALUATION_SHADER));
	CHECK_GL_ERROR(glShaderSource(ocean_te_shader_id, 1,
				&ocean_te_source_pointer, nullptr));
	glCompileShader(ocean_te_shader_id);
	CHECK_GL_SHADER_ERROR(ocean_te_shader_id);

	GLuint ocean_fragment_shader_id = 0;
	const char* ocean_fragment_source_pointer = ocean_fragment_shader;
	CHECK_GL_ERROR(ocean_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(ocean_fragment_shader_id, 1,
				&ocean_fragment_source_pointer, nullptr));
	glCompileShader(ocean_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(ocean_fragment_shader_id);

	// Setup ocean_geometry shader.
	GLuint ocean_geometry_shader_id = 0;
	const char* ocean_geometry_source_pointer = ocean_geometry_shader;
	CHECK_GL_ERROR(ocean_geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
	CHECK_GL_ERROR(glShaderSource(ocean_geometry_shader_id, 1, &ocean_geometry_source_pointer, nullptr));
	glCompileShader(ocean_geometry_shader_id);
	CHECK_GL_SHADER_ERROR(ocean_geometry_shader_id);

	// FIXME: Setup another program for the OCEAN, and get its locations.
	// Note: you can reuse the vertex and geometry shader objects

		// Let's create our program.
	GLuint ocean_program_id = 0;
	CHECK_GL_ERROR(ocean_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, ocean_fragment_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, ocean_geometry_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, ocean_tc_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, ocean_te_shader_id));

	// Bind attributes.
	//CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	CHECK_GL_ERROR(glBindAttribLocation(ocean_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(ocean_program_id, 0, "fragment_color"));
	glLinkProgram(ocean_program_id);
	CHECK_GL_PROGRAM_ERROR(ocean_program_id);

	// Get the uniform locations.
	GLint ocean_projection_matrix_location = 0;
	CHECK_GL_ERROR(ocean_projection_matrix_location =
			glGetUniformLocation(ocean_program_id, "projection"));
	GLint ocean_view_matrix_location = 0;
	CHECK_GL_ERROR(ocean_view_matrix_location =
			glGetUniformLocation(ocean_program_id, "view"));
	GLint ocean_light_position_location = 0;
	CHECK_GL_ERROR(ocean_light_position_location =
			glGetUniformLocation(ocean_program_id, "light_position"));
	GLint ocean_eye_position_location = 0;
	CHECK_GL_ERROR(ocean_eye_position_location =
			glGetUniformLocation(ocean_program_id, "eye_position"));

	GLint ocean_wireframes_location = 0;
	CHECK_GL_ERROR(ocean_wireframes_location =
			glGetUniformLocation(ocean_program_id, "show_wireframes"));
	GLint ocean_inner_level_location = 0;
	CHECK_GL_ERROR(ocean_inner_level_location =
			glGetUniformLocation(ocean_program_id, "inner_level"));
	GLint ocean_outer_level_location = 0;
	CHECK_GL_ERROR(ocean_outer_level_location =
			glGetUniformLocation(ocean_program_id, "outer_level"));
	GLint t_location = 0;
	CHECK_GL_ERROR(t_location =
			glGetUniformLocation(ocean_program_id, "t"));
	GLint wave_x_location = 0;
	CHECK_GL_ERROR(wave_x_location =
			glGetUniformLocation(ocean_program_id, "wave_x"));

	//Create Sphere Program
	GLuint sphere_program_id = 0;
	CHECK_GL_ERROR(sphere_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(sphere_program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(sphere_program_id, sphere_fragment_shader_id));
	CHECK_GL_ERROR(glAttachShader(sphere_program_id, geometry_shader_id));
	//CHECK_GL_ERROR(glAttachShader(sphere_program_id, floor_tc_shader_id));
	//CHECK_GL_ERROR(glAttachShader(sphere_program_id, floor_te_shader_id));

	// Bind attributes.
	//CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	CHECK_GL_ERROR(glBindAttribLocation(sphere_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(sphere_program_id, 0, "fragment_color"));
	glLinkProgram(sphere_program_id);
	CHECK_GL_PROGRAM_ERROR(sphere_program_id);

	// Get the uniform locations.
	GLint sphere_projection_matrix_location = 0;
	CHECK_GL_ERROR(sphere_projection_matrix_location =
			glGetUniformLocation(sphere_program_id, "projection"));
	GLint sphere_view_matrix_location = 0;
	CHECK_GL_ERROR(sphere_view_matrix_location =
			glGetUniformLocation(sphere_program_id, "view"));
	GLint sphere_light_position_location = 0;
	CHECK_GL_ERROR(sphere_light_position_location =
			glGetUniformLocation(sphere_program_id, "light_position"));

	//glm::vec4 light_position = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f); Old Light Source
	glm::vec4 light_position = glm::vec4(-10.0f, 10.0f, 0.0f, 1.0f);
	float aspect = 0.0f;
	float theta = 0.0f;

	auto start = std::chrono::system_clock::now();
	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);

		std::chrono::duration<float> diff = std::chrono::system_clock::now()-start;
		t = diff.count();
		//CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));
		//CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kVertexBuffer]));
		// // NOTE: We do not send anything right now, we just describe it to OpenGL.
		// glBufferSubData(GL_ARRAY_BUFFER, 0,
		// 			sizeof(float) * ocean_vertices.size() * 4, ocean_vertices.data());

		// Switch to the VAO for Floor.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

		// Generate buffer objects
		CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kOceanVao][0]));;

		// Setup vertex data in a VBO.
		CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kVertexBuffer]));
		// NOTE: We do not send anything right now, we just describe it to OpenGL.
		CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
					sizeof(float) * ocean_vertices.size() * 4, ocean_vertices.data(),
					GL_DYNAMIC_DRAW));
		CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
		CHECK_GL_ERROR(glEnableVertexAttribArray(0));

		// Setup element array buffer.
		CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kIndexBuffer]));
		CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					sizeof(uint32_t) * ocean_faces.size() * 4,
					ocean_faces.data(), GL_DYNAMIC_DRAW));

		// Switch to the Geometry VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		if (g_menger && g_menger->is_dirty()) {
			g_menger->generate_geometry(obj_vertices, obj_faces);
			g_menger->set_clean();

			// FIXME: Upload your vertex data here.
			CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
			CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
			// NOTE: We do not send anything right now, we just describe it to OpenGL.
			CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
						sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(),
						GL_STATIC_DRAW));
			CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
			CHECK_GL_ERROR(glEnableVertexAttribArray(0));

			// Setup element array buffer.
			CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						sizeof(uint32_t) * obj_faces.size() * 3,
						obj_faces.data(), GL_STATIC_DRAW));

			//Floor
			// CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));;

			// CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));

			// CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));
			// // NOTE: We do not send anything right now, we just describe it to OpenGL.
			// CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
			// 			sizeof(float) * floor_vertices.size() * 4, floor_vertices.data(),
			// 			GL_STATIC_DRAW));
			// CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
			// CHECK_GL_ERROR(glEnableVertexAttribArray(0));

			// // Setup element array buffer.
			// CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kIndexBuffer]));
			// CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			// 			sizeof(uint32_t) * floor_faces.size() * 3,
			// 			floor_faces.data(), GL_STATIC_DRAW));

			//OCEAN after hitting keypad
			// CreateOcean(ocean_vertices, ocean_faces, t.count());
			// // Switch to the VAO for Floor.
			// CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

			// // Setup vertex data in a VBO.
			// CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kVertexBuffer]));
			// CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kIndexBuffer]));

			// // NOTE: We do not send anything right now, we just describe it to OpenGL.
			// CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
			// 			sizeof(float) * ocean_vertices.size() * 4, ocean_vertices.data(),
			// 			GL_DYNAMIC_DRAW));
			// CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
			// CHECK_GL_ERROR(glEnableVertexAttribArray(0));

			// // Setup element array buffer.
			// CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			// 			sizeof(uint32_t) * ocean_faces.size() * 4,
			// 			ocean_faces.data(), GL_DYNAMIC_DRAW));

		}

		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		glm::mat4 projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

		// Compute the view matrix
		// FIXME: change eye and center through mouse/keyboard events.
		glm::mat4 view_matrix = g_camera.get_view_matrix();
		glm::vec3 eye = g_camera.get_eye();

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));

		// Draw our triangles.
		if(polycount == 3){
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));
		}
		

		// FIXME: Render the floor
		// Note: What you need to do is
		// 	1. Switch VAO
		// 	2. Switch Program
		// 	3. Pass Uniforms
		// 	4. Call glDrawElements, since input geometry is
		// 	indicated by VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));
		CHECK_GL_ERROR(glUseProgram(floor_program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(floor_projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(floor_view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(floor_light_position_location, 1, &light_position[0]));
		CHECK_GL_ERROR(glUniform1i(wireframes_location, wireframes));
		CHECK_GL_ERROR(glUniform1i(inner_level_location, inner_level));
		CHECK_GL_ERROR(glUniform1i(outer_level_location, outer_level));

		// Draw our triangles.
		if(false){
			CHECK_GL_ERROR(glDrawElements(GL_PATCHES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
		}

		//Render Sphere
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kSphereVao]));
		CHECK_GL_ERROR(glUseProgram(sphere_program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(sphere_projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(sphere_view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(sphere_light_position_location, 1, &light_position[0]));
		//CHECK_GL_ERROR(glUniform1i(wireframes_location, wireframes));
		//CHECK_GL_ERROR(glUniform1i(inner_level_location, inner_level));
		//CHECK_GL_ERROR(glUniform1i(outer_level_location, outer_level));

		// Draw our triangles.
		if(show_sphere){
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, sphere_faces.size() * 3, GL_UNSIGNED_INT, 0));
		}



		// FIXME: Render the Ocean
		// Note: What you need to do is
		// 	1. Switch VAO
		// 	2. Switch Program
		// 	3. Pass Uniforms
		// 	4. Call glDrawElements, since input geometry is
		// 	indicated by VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));
		CHECK_GL_ERROR(glUseProgram(ocean_program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(ocean_projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(ocean_view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(ocean_light_position_location, 1, &light_position[0]));
		CHECK_GL_ERROR(glUniform3fv(ocean_eye_position_location, 1, &eye[0]));
		CHECK_GL_ERROR(glUniform1i(ocean_wireframes_location, wireframes));
		CHECK_GL_ERROR(glUniform1i(ocean_inner_level_location, inner_level));
		CHECK_GL_ERROR(glUniform1i(ocean_outer_level_location, outer_level));
		CHECK_GL_ERROR(glUniform1f(t_location, t));

		float wave_x = (t - wave_time > 100) ? 100 : t - wave_time;
		CHECK_GL_ERROR(glUniform1f(wave_x_location, wave_x));


		// Draw our triangles.
		if(polycount == 4){
			CHECK_GL_ERROR(glDrawElements(GL_PATCHES, ocean_faces.size() * 4, GL_UNSIGNED_INT, 0));
		}

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
