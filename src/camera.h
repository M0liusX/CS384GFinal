#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

class Camera {
public:
	glm::mat4 get_view_matrix() const;
  void rotate(double mouse_x, double mouse_y);
  void roll(int right);
  void forward(int forward);
  void strafe(int right);
  void up(int up);
  glm::vec3 get_eye();

private:
	float camera_distance_ = 10.0;
	glm::vec3 look_ = glm::normalize(glm::vec3(0.0f, -camera_distance_, -camera_distance_));
	glm::vec3 up_ = glm::vec3(0.0f, 1.0, 0.0f);
	glm::vec3 eye_ = glm::vec3(0.0f, camera_distance_, camera_distance_);
	// Note: you may need additional member variables
  glm::vec3 center_ = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 tangent_ = glm::vec3(1.0f, 0.0f, 0.0f);
};

#endif
