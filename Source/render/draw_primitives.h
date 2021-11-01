#pragma once

// Create vertex buffer
struct VtxPosColor {
  VEC3 p; 
  VEC4 color;
  VtxPosColor() = default;
  VtxPosColor(float px, float py, float pz, VEC4 new_color)
    : p(px, py, pz), color(new_color) {}
  VtxPosColor(VEC3 new_p, VEC4 new_color) : p(new_p), color(new_color) {}
};

struct VtxPosNUvT {
  VEC3 pos;
  VEC3 normal;
  VEC2 uv;
  VEC3 tangent;
  float tangent_w;
};

struct VtxPosUv {
  VEC3 pos;
  VEC2 uv;
  VtxPosUv() = default;
  VtxPosUv(float px, float py, float pz, VEC2 new_uv)
    : pos(px, py, pz), uv(new_uv) {
  }
};


bool createAndRegisterPrimitives();

class CCamera;
void activateCamera(const CCamera& camera);
void activateObject(const MAT44& world, VEC4 color = Colors::White, CHandle hEntity = CHandle());

void drawLine(VEC3 src, VEC3 dst, VEC4 color = Colors::White);
void drawAxis(const MAT44& transform);
void drawWiredAABB(AABB aabb, const MAT44 world, VEC4 color = Colors::White);
void drawText3D(const VEC3& world_coord, VEC4 color, const char* text, float font_size = 0.0f);
void drawWiredSphere(const MAT44 world, float radius, VEC4 color = Colors::White);
void drawPrimitive(const CMesh* mesh, MAT44 world, VEC4 color = Colors::White, RSConfig rs = RSConfig::DEFAULT);
void drawFullScreenQuad(const std::string& pipeline_name, const CTexture* extra_texture = nullptr, const CTexture* extra_texture2 = nullptr);

void drawBox(VEC3 center, VEC3 half_size, const MAT44 world, VEC4 color = Colors::White);
void drawCone(CTransform* trans, float cone_fov, float cone_length);
void drawCircle(VEC3 center, QUAT rotation, float radius, int sides, VEC4 color);
void drawProgressBar3D(const VEC3& world_coord, VEC4 color, float value, float value_max, VEC2 offset2d = VEC2::Zero);
void drawProgressBar2D(const VEC2& screen_cord, VEC4 color, float value, float value_max, const char* string = nullptr, float width = 120, float height = 20, bool bottom = true, 
    float lerp_value = -1, Color lerp_color = Colors::White);
void drawText2D(const VEC2& screen_cord, VEC4 color, const char* text, bool center_x = false, bool bottom = true, float font_size = 0.0f, bool remove_bck = false);
void drawSpinner(const VEC2& screen_cord, VEC4 color, float value, float value_max, float radius, float thickness, bool bottom = true);
void drawCircle3D(const VEC3& world_coord, VEC4 color, float radius, int num_segments = 0);

const CCamera& getActiveCamera();
