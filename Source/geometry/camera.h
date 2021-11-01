#pragma once

class CCamera {

protected:

  MAT44 view;
  MAT44 projection;
  MAT44 view_projection;

  VEC3  eye;
  VEC3  target;
  VEC3  front;
  VEC3  left;
  VEC3  up;

  float fov_vertical_radians = deg2rad(43.0f);
  float aspect_ratio = 1.0f;
  float z_min = 0.1f;
  float z_max = 1000.0f;

  // Ortho params
  float ortho_left = 0.f;
  float ortho_top = 0.f;
  float ortho_width = 1.0f;
  float ortho_height = 1.0f;
  bool  ortho_centered = false;

  bool is_ortho = false;

  void updateViewProjection();

public:

  const MAT44& getView() const { return view; }
  const MAT44& getProjection() const { return projection; }
  const MAT44& getViewProjection() const { return view_projection; }

  VEC3 getForward() const { return front; }
  VEC3 getUp() const { return up; }
  VEC3 getRight() const { return -left; }
  VEC3 getEye() const { return eye; }

  float getAspectRatio() const { return aspect_ratio; }
  float getFov() const { return fov_vertical_radians; }
  float getNear() const { return z_min; }
  float getFar() const { return z_max; }

  float getFovVerticalRadians() const { return fov_vertical_radians; }
  void setFovVerticalRadians(float new_fov_vertical_radians) { fov_vertical_radians = new_fov_vertical_radians; }
  void setAspectRatio(float new_ratio);
  void setFov(float new_fov_vertical_radians);

  // ...
  void lookAt(VEC3 new_eye, VEC3 new_target, VEC3 new_up_aux = VEC3(0,1,0));
  void setProjectionParams(float new_fov_vertical_radians, float new_aspect_ratio, float new_z_near, float new_z_far);
  void setOrthoParams(bool is_centered, float offset_left, float width, float offset_top, float height, float new_znear, float new_zfar);
  
  VEC3 getTarget() const;

  // Projections
  VEC2 project(VEC3 world_pos, const VEC2& resolution = -VEC2::One);
  VEC3 unproject(VEC2 screen_pos);

};
