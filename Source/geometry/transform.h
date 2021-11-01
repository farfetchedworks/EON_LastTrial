#pragma once

class CTransform {

  QUAT  rotation;
  VEC3  position;
  VEC3  scale = VEC3::One;
  
public:

  void setRotation(QUAT new_rotation) { rotation = new_rotation; }
  void setPosition(VEC3 new_position) { position = new_position; }
  void setScale(VEC3 new_scale) { scale = new_scale; }

  QUAT getRotation() const { return rotation; }
  VEC3 getPosition() const { return position; }
  VEC3 getScale() const { return scale; }

  VEC3 getForward() const;
  VEC3 getUp() const;
  VEC3 getRight() const;

  MAT44 asMatrix() const;
  void fromMatrix(MAT44 mtx);
  void fromMatrix(CTransform t);
  
  CTransform combinedWith(const CTransform& delta_transform) const;
  bool isInFront(VEC3 p) const;
  bool isInsideCone(VEC3 p, float full_fov_radians, float z_cone) const;
  float getYawRotationToAimTo(VEC3 p) const;
  float getPitchRotationToAimTo(VEC3 p) const;

  float distance(const CTransform& t) const;

  void setEulerAngles(float yaw, float pitch, float roll);
  void getEulerAngles(float* yaw, float* pitch, float* roll) const;

  void lookAt(VEC3 src, VEC3 target, VEC3 up_aux);
  bool fromJson(const json& j);

  // read/write from json
  // imgui
  bool renderInMenu();
  bool renderGuizmo();

  void interpolateTo(const CTransform& target, float amount_of_target);
};
