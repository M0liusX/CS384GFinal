#include "camera.h"

namespace {
	float pan_speed = 0.1f;
	float roll_speed = 0.1f;
	float rotation_speed = 0.05f;
	float zoom_speed = 0.1f;
};

glm::mat4 Camera::get_view_matrix() const
{
  glm::mat4 view_matrix;
  glm::vec3 Z = glm::normalize(eye_ - center_);
  glm::vec3 X = glm::normalize(glm::cross(up_, Z));
  glm::vec3 Y = glm::normalize(glm::cross(Z, X));
  view_matrix[0] = glm::vec4(X,-glm::dot(X, eye_));
  view_matrix[1] = glm::vec4(Y,-glm::dot(Y, eye_));
  view_matrix[2] = glm::vec4(Z,-glm::dot(Z, eye_));
  view_matrix[3] = glm::vec4(0,0,0,1);

  view_matrix = glm::transpose(view_matrix);
  return view_matrix;
}

void 
Camera::rotate(double delta_x, double delta_y) {

  // float angle = pow(pow(delta_x,2) + pow(delta_y,2), 0.5);
  // glm::vec3 direction = glm::normalize((float)delta_x * -tangent_ + (float)delta_y * up_);
  // glm::vec3 axis = glm::normalize(glm::cross(look_, direction));
  
  // look_ =  glm::normalize(glm::rotate(look_, (float)angle* rotation_speed, axis));
  // up_ =  glm::rotate(up_, (float)angle* rotation_speed, axis);
  // tangent_ =  glm::rotate(tangent_, (float)angle* rotation_speed, axis);
  
  look_ =  glm::normalize(glm::rotate(look_, (float)delta_x * rotation_speed, glm::vec3(0.0f, 1.0f, 0.0f)));
  //tangent_   =  glm::normalize(glm::rotate(tangent_, (float)delta_x * rotation_speed, glm::vec3(0.0f, 1.0, 0.0f)));
  up_   =  glm::normalize(glm::rotate(up_, (float)delta_x * rotation_speed, glm::vec3(0.0f, 1.0, 0.0f)));
  tangent_ = glm::normalize(glm::cross(look_, up_));

  look_ =  glm::normalize(glm::rotate(look_, (float)delta_y * rotation_speed, glm::vec3(1.0f, 0.0, 0.0f)));
  //up_ =  glm::normalize(glm::rotate(up_, (float)delta_y * rotation_speed, glm::vec3(1.0f, 0.0, 0.0f)));
  //tangent_ = glm::normalize(glm::cross(look_, up_));
  tangent_ = glm::normalize(glm::rotate(tangent_, (float)delta_y * rotation_speed, glm::vec3(1.0f, 0.0, 0.0f)));
  up_ = glm::normalize(glm::cross(look_, -tangent_));
  
  center_ = look_ * camera_distance_ + eye_;
}

void
Camera::forward(int forward) {
  glm::vec3 scale = look_ * zoom_speed;

  float dist = glm::length(center_ - eye_);
  camera_distance_ = dist - forward*zoom_speed;
  eye_ += (float)forward*scale;

}

void
Camera::strafe(int right) {
  center_ += (float)right*tangent_ * pan_speed;
  eye_ += (float)right*tangent_ * pan_speed;
}

void
Camera::roll(int right) {
  up_ = glm::normalize(glm::rotate(up_, (float)right*roll_speed, look_));
  tangent_ = glm::normalize(glm::cross(look_, up_));
}

void
Camera::up(int up) {
  center_ += (float)up*up_ * pan_speed;
  eye_ += (float)up*up_ * pan_speed;

}

glm::vec3
Camera::get_eye(){
  return eye_;
}