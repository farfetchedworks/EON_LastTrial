#include "mcv_platform.h"
#include "camera.h"

void CCamera::updateViewProjection() {
  view_projection = view * projection;
}

void CCamera::lookAt(VEC3 new_eye, VEC3 new_target, VEC3 new_up_aux) {
  eye = new_eye;
  target = new_target;
  view = MAT44::CreateLookAt(new_eye, new_target, new_up_aux);
  updateViewProjection();

  front = (new_target - new_eye);
  front.Normalize();
  left = new_up_aux.Cross(front);
  left.Normalize();
  up = front.Cross(left);
}

void CCamera::setProjectionParams(float new_fov_vertical_radians, float new_aspect_ratio, float new_z_near, float new_z_far) {
  z_min = new_z_near;
  z_max = new_z_far;
  fov_vertical_radians = new_fov_vertical_radians;
  aspect_ratio = new_aspect_ratio;
  projection = MAT44::CreatePerspectiveFieldOfView(
    new_fov_vertical_radians, 
    new_aspect_ratio, 
    new_z_near, new_z_far
  );
  updateViewProjection();

  is_ortho = false;
}

void CCamera::setOrthoParams(bool is_centered, float offset_left, float width, float offset_top, float height, float new_znear, float new_zfar)
{
  z_min = new_znear;
  z_max = new_zfar;
  aspect_ratio = width/height;
  ortho_centered = is_centered;
  ortho_width = width;
  ortho_height = height;
  ortho_left = offset_left;
  ortho_top = offset_top;

  if (ortho_centered)
  {
      projection = MAT44::CreateOrthographic(ortho_width, ortho_height, z_min, z_max);
  }
  else
  {
      projection = MAT44::CreateOrthographicOffCenter(ortho_left, ortho_width, ortho_height, ortho_top, z_min, z_max);
  }
  updateViewProjection();
}

VEC3 CCamera::getTarget() const
{
  return target;
}

void CCamera::setAspectRatio(float new_aspect_ratio) {
  setProjectionParams(fov_vertical_radians, new_aspect_ratio, z_min, z_max);
}

void CCamera::setFov(float new_fov_vertical_radians) {
  setProjectionParams(new_fov_vertical_radians, aspect_ratio, z_min, z_max);
}

VEC2 CCamera::project(VEC3 world_pos, const VEC2& resolution)
{
    const MAT44& view_proj = getViewProjection();

    // Because it divides by w
    // It's in the range -1..1
    VEC3 homo_pos = VEC3::Transform(world_pos, view_proj);

    if (homo_pos.z <= 0.0f || homo_pos.z >= 1.0f)
        return VEC2(-1.f);

    bool useScreenResolution = (resolution == -VEC2::One);

    float render_width = useScreenResolution ? (float)Render.getWidth() : resolution.x;
    float render_height = useScreenResolution ? (float)Render.getHeight() : resolution.y;

    // Convert -1..1 to 0..1024
    float sx = (homo_pos.x + 1.f) * 0.5f * render_width;
    float sy = (1.0f - homo_pos.y) * 0.5f * render_height;

    return VEC2(sx, sy);
}

VEC3 CCamera::unproject(VEC2 screen_pos)
{
    float width = (float)Render.getWidth();
    float height = (float)Render.getHeight();
    screen_pos.x = clampf(screen_pos.x, 0.f, width);
    screen_pos.y = clampf(screen_pos.y, 0.f, height);

    VEC4 screen_space_pos = VEC4(
        screen_pos.x * 2.f / width - 1.f,
        -(screen_pos.y * 2.f / height - 1.f),
        -1.f,
        1.f
    );
   
    MAT44 inv_vp;
    getViewProjection().Invert(inv_vp);

    VEC4 homo_space_pos = VEC4::Transform(screen_space_pos, inv_vp);
    assert(homo_space_pos.w != 0.f);

    /*assert(homo_space_pos.x >= -1.f && homo_space_pos.x <= 1.f
        && homo_space_pos.y >= -1.f && homo_space_pos.y <= 1.f
        && homo_space_pos.z >= 0.f && homo_space_pos.z <= 1.f);*/

    VEC3 world_pos = VEC3(homo_space_pos) / homo_space_pos.w;
    return world_pos;
}