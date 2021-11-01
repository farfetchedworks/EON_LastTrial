#include "mcv_platform.h"
#include "draw_primitives.h"
#include "geometry/geometry.h"
#include "components/render/comp_dissolve.h"
#include "imgui/imgui_internal.h"

extern CShaderCte<CtesCamera> cte_camera;
extern CShaderCte<CtesObject> cte_object;

extern CMesh* line = nullptr;
extern CMesh* axis = nullptr;
extern CMesh* grid = nullptr;
extern CMesh* unit_wired_cube = nullptr;
extern CMesh* unit_wired_circle_xz = nullptr;
extern CMesh* view_volume_wired = nullptr;

CCamera the_active_camera;

static void createAxis(CMesh& mesh) {
  VtxPosColor vertices[6] = {
    { VEC3(0,0,0), Colors::Red },
    { VEC3(1,0,0), Colors::Red },
    { VEC3(0,0,0), Colors::Green },
    { VEC3(0,2,0), Colors::Green },
    { VEC3(0,0,0), Colors::Blue },
    { VEC3(0,0,3), Colors::Blue },
  };
  mesh.create(
    vertices, sizeof(vertices), sizeof(VtxPosColor),
    nullptr, 0, 0,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST
  );
}

static void createGrid(CMesh& mesh, int npoints) {
  std::vector<VtxPosColor> vertices;

  VEC4 clrA(0.5f, 0.5f, 0.5f, 1.0f);
  VEC4 clrB(0.25f, 0.25f, 0.25f, 1.0f);
  float maxi = (float)npoints;
  for (int i = -npoints; i <= npoints; ++i) {
    VEC4 clr = (i % 5) == 0 ? clrA : clrB;
    vertices.emplace_back(VEC3(-maxi, 0, (float)i), clr);
    vertices.emplace_back(VEC3(maxi, 0, (float)i), clr);
    vertices.emplace_back(VEC3((float)i, 0, -maxi), clr);
    vertices.emplace_back(VEC3((float)i, 0, maxi), clr);
  }

  mesh.create(
    vertices.data(), vertices.size() * sizeof(VtxPosColor), sizeof(VtxPosColor),
    nullptr, 0, 0,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST
  );
}

static void createLine(CMesh& mesh) {
  VtxPosColor vertices[2];
  vertices[0] = VtxPosColor(VEC3(0, 0, 0), Colors::White);
  vertices[1] = VtxPosColor(VEC3(0, 0, 1), Colors::White);
  mesh.create(
    vertices, 2 * sizeof(VtxPosColor), sizeof(VtxPosColor),
    nullptr, 0, 0,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST
  );
}

static void createUnitWiredCube(CMesh& mesh) {
  std::vector<VtxPosColor> vtxs =
  {
    VtxPosColor(-0.5f,-0.5f, -0.5f,  Colors::White),    // 
    VtxPosColor( 0.5f,-0.5f, -0.5f,  Colors::White),
    VtxPosColor(-0.5f, 0.5f, -0.5f,  Colors::White),
    VtxPosColor( 0.5f, 0.5f, -0.5f,  Colors::White),    // 
    VtxPosColor(-0.5f,-0.5f, 0.5f,   Colors::White),    // 
    VtxPosColor( 0.5f,-0.5f, 0.5f,   Colors::White),
    VtxPosColor(-0.5f, 0.5f, 0.5f,   Colors::White),
    VtxPosColor( 0.5f, 0.5f, 0.5f,   Colors::White),    // 
  };
  const std::vector<uint16_t> idxs = {
      0, 1, 2, 3, 4, 5, 6, 7
    , 0, 2, 1, 3, 4, 6, 5, 7
    , 0, 4, 1, 5, 2, 6, 3, 7
  };
  const int nindices = 8 * 3;
  mesh.create(
    vtxs.data(), (uint32_t)vtxs.size() * sizeof(VtxPosColor), sizeof(VtxPosColor),
    idxs.data(), nindices * sizeof(uint16_t), sizeof(uint16_t),
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST
  );
}

static void createUnitWiredCircleXZ(CMesh& mesh)
{
  const int samples = 32;
  std::vector<uint16_t> idxs;
  std::vector< VtxPosColor > vtxs;
  vtxs.reserve(samples);
  for (int i = 0; i <= samples; ++i)
  {
    float angle = 2 * (float)M_PI * (float)i / (float)samples;
    VtxPosColor v1;
    v1.p = yawToVector(angle);
    v1.color = Colors::White;
    vtxs.push_back(v1);
    idxs.push_back(i);
  }
  mesh.create(
    vtxs.data(), (uint32_t)vtxs.size() * sizeof(VtxPosColor), sizeof(VtxPosColor),
    idxs.data(), idxs.size() * sizeof(uint16_t), sizeof(uint16_t),
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP
  );
}

static void createViewVolumeWired(CMesh& mesh) {
  std::vector<VtxPosColor> vtxs =
  {
    VtxPosColor(-1.0f,-1.0f, 0.0f,  Colors::White),    // 
    VtxPosColor(1.0f,-1.0f, 0.0f,  Colors::White),
    VtxPosColor(-1.0f, 1.0f, 0.0f,  Colors::White),
    VtxPosColor(1.0f, 1.0f, 0.0f,  Colors::White),    // 
    VtxPosColor(-1.0f,-1.0f, 1.0f,  Colors::White),    // 
    VtxPosColor(1.0f,-1.0f, 1.0f,  Colors::White),
    VtxPosColor(-1.0f, 1.0f, 1.0f,  Colors::White),
    VtxPosColor(1.0f, 1.0f, 1.0f,  Colors::White),    // 
  };
  const std::vector<uint16_t> idxs = {
      0, 1, 2, 3, 4, 5, 6, 7
    , 0, 2, 1, 3, 4, 6, 5, 7
    , 0, 4, 1, 5, 2, 6, 3, 7
  };
  const int nindices = 8 * 3;
  mesh.create(
    vtxs.data(), (uint32_t)vtxs.size() * sizeof(VtxPosColor), sizeof(VtxPosColor),
    idxs.data(), nindices * sizeof(uint16_t), sizeof(uint16_t),
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST
  );

}

// ------------------------------------------------
static void createUnitQuadXY(CMesh& mesh) {
  VtxPosUv vtxs[4];
  vtxs[0] = VtxPosUv(0, 0, 0, VEC2(0, 0));
  vtxs[1] = VtxPosUv(1, 0, 0, VEC2(1, 0));
  vtxs[2] = VtxPosUv(1, 1, 0, VEC2(1, 1));
  vtxs[3] = VtxPosUv(0, 1, 0, VEC2(0, 1));

  // Create some idxs 0,1,2,3...
  const int nindices = 6;
  uint16_t idxs[nindices] = {
    0, 1, 2,
    2, 3, 0,
  };

  mesh.create(
    vtxs, 4 * sizeof(VtxPosUv), sizeof(VtxPosUv),
    idxs, nindices * sizeof(uint16_t), sizeof(uint16_t),
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    getVertexDeclarationByName("PosUv")
  );
}



bool createAndRegisterPrimitives() {
  PROFILE_FUNCTION("createAndRegisterPrimitives");
  {
    CMesh* mesh = new CMesh;
    createAxis(*mesh);
    Resources.registerResource(mesh, "axis.mesh", getClassResourceType<CMesh>());
    axis = mesh;
  }
  {
    CMesh* mesh = new CMesh;
    createGrid(*mesh, 10);
    Resources.registerResource(mesh, "grid.mesh", getClassResourceType<CMesh>());
    grid = mesh;
  }
  {
    CMesh* mesh = new CMesh;
    createLine(*mesh);
    Resources.registerResource(mesh, "line.mesh", getClassResourceType<CMesh>());
    line = mesh;
  }
  {
    CMesh* mesh = new CMesh;
    createUnitWiredCube(*mesh);
    Resources.registerResource(mesh, "unit_wired_cube.mesh", getClassResourceType<CMesh>());
    unit_wired_cube = mesh;
  }
  {
    CMesh* mesh = new CMesh;
    createUnitWiredCircleXZ(*mesh);
    Resources.registerResource(mesh, "unit_wired_circle_xz.mesh", getClassResourceType<CMesh>());
    unit_wired_circle_xz = mesh;
  }
  {
    CMesh* mesh = new CMesh;
    createViewVolumeWired(*mesh);
    Resources.registerResource(mesh, "view_volume_wired.mesh", getClassResourceType<CMesh>());
    view_volume_wired = mesh;
  }
  {
    CMesh* mesh = new CMesh;
    createUnitQuadXY(*mesh);
    Resources.registerResource(mesh, "unit_quad_xy.mesh", getClassResourceType<CMesh>());
  }
  return true;
}

// Gets all the information from the camera obj and uploads it to the GPU cte buffer 
// associated to the camera
void activateCamera(const CCamera& camera) {
  the_active_camera = camera;
  cte_camera.camera_view = camera.getView();
  cte_camera.camera_projection = camera.getProjection();
  cte_camera.camera_view_projection = camera.getViewProjection();
  cte_camera.camera_inverse_view_projection = cte_camera.camera_view_projection.Invert();
  cte_camera.camera_forward = camera.getForward();
  cte_camera.camera_zfar = camera.getFar();
  cte_camera.camera_position = camera.getEye();
  cte_camera.camera_tan_half_fov = tanf( camera.getFov() * 0.5f );
  cte_camera.camera_aspect_ratio = camera.getAspectRatio();
  cte_camera.camera_znear = camera.getNear();
  cte_camera.camera_right = camera.getRight();
  cte_camera.camera_up = camera.getUp();
  cte_camera.updateFromCPU();
}

const CCamera& getActiveCamera() {
  return the_active_camera;
}

void activateObject(const MAT44& world, VEC4 color, CHandle hEntity) {
  cte_object.object_world = world;
  cte_object.object_color = color;
  cte_object.object_dissolve_timer = 0.f;
  cte_object.object_max_dissolve_time = 0.f;
  cte_object.object_dummy1 = 0.f;
  cte_object.object_dummy2 = 0.f;

  CEntity* entity = hEntity;
  if(entity) {
      TCompDissolveEffect* c_dissolve = entity->get<TCompDissolveEffect>();
      if (c_dissolve)
          c_dissolve->updateObjectCte(cte_object);
  }

  cte_object.updateFromCPU();
}

void drawLine(VEC3 src, VEC3 dst, VEC4 color) {
  VEC3 delta = dst - src;
  float d = delta.Length();
  if (d < 1e-3f)
    return;
  if (fabsf(delta.x) < 1e-5f && fabsf(delta.z) < 1e-5f) 
    dst.x += 1e-3f;
  MAT44 world = MAT44::CreateScale(1, 1, -d) * MAT44::CreateLookAt(src, dst, VEC3(0, 1, 0)).Invert();
  activateObject(world, color);
  line->activate();
  line->render();
}

void drawAxis(const MAT44& transform) {
  activateObject(transform, Colors::White);
  axis->activate();
  axis->render();
}

void drawWiredAABB(AABB aabb, const MAT44 world, VEC4 color) {
  MAT44 unit_cube_to_world = 
    MAT44::CreateScale( VEC3(aabb.Extents) * 2.0f ) 
    * MAT44::CreateTranslation( aabb.Center )
    * world;
  drawPrimitive(unit_wired_cube, unit_cube_to_world, color);
}

void drawBox(VEC3 center, VEC3 half_size, const MAT44 world, VEC4 color) {
    MAT44 unit_cube_to_world =
        MAT44::CreateScale(half_size * 2.0f)
        * MAT44::CreateTranslation(center)
        * world;
    drawPrimitive(unit_wired_cube, unit_cube_to_world, color);
}

void drawWiredSphere(const MAT44 user_world, float radius, VEC4 color) {
  MAT44 world = MAT44::CreateScale(radius) * user_world;
  const CMesh* mesh = unit_wired_circle_xz;
  // Draw 3 circleXZ 
  drawPrimitive(mesh, world, color);
  drawPrimitive(mesh, MAT44::CreateRotationX((float)M_PI_2) * world, color);
  drawPrimitive(mesh, MAT44::CreateRotationZ((float)M_PI_2) * world, color);
}

void drawPrimitive(const CMesh* mesh, MAT44 world, VEC4 color, RSConfig rs) {
  
  assert(mesh);
  auto vdecl = mesh->getVertexDecl();
  if (vdecl->name == "Pos") {
    const CPipelineState* pipe = Resources.get("debug_white.pipeline")->as<CPipelineState>();
    pipe->activate();
  }
  else if (rs != RSConfig::DEFAULT) {
      activateRSConfig(rs); 
  }
  else if(vdecl->name != "PosClr")
  {
    const CPipelineState* pipe = Resources.get("debug_white.pipeline")->as<CPipelineState>();
    pipe->activate();
  }

  activateObject(world, color);

  mesh->activate();
  mesh->render();
}

void drawFullScreenQuad(const std::string& pipeline_name, const CTexture* extra_texture, const CTexture* extra_texture2) {
    PROFILE_FUNCTION("FullQUad");
    CGpuScope gpu_scope(pipeline_name.c_str());
    // from name to CPipelineState
    auto* pipeline = Resources.get(pipeline_name)->as<CPipelineState>();
    assert(pipeline);
    pipeline->activate();
    if (extra_texture)
        extra_texture->activate(TS_ALBEDO);
    if (extra_texture2)
        extra_texture2->activate(TS_NORMAL);
    auto* mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();
    mesh->activate();
    mesh->render();
}

void drawText2D(const VEC2& screen_cord, VEC4 color, const char* text, bool center_x, bool bottom, float font_size, bool remove_bck)
{
    ImVec2 sz = ImGui::CalcTextSize(text);
    int margin = 4;

    float render_width = (float)Render.getWidth();
    float render_height = (float)Render.getHeight();

    float sx = screen_cord.x;
    float sy = screen_cord.y;

    if (center_x)
    {
        sx = (render_width / 2.f) - (sz.x / 2.f);
    }

    if (bottom) {
        sy = render_height - screen_cord.y;
        sy -= (sz.y - margin) * 2.f;
    }

    auto* dl = ImGui::GetForegroundDrawList();

    if(!remove_bck)
        dl->AddRectFilled(ImVec2(sx - margin, sy - margin), ImVec2(sx + sz.x + margin, sy + sz.y + margin), 0xa0000000, 2.f);

    // dl->AddText(ImVec2(sx, sy), ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), text);
    dl->AddText(NULL, font_size, ImVec2(sx, sy), ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), text);
}

void drawSpinner(const VEC2& screen_cord, VEC4 color, float value, float value_max, float radius, float thickness, bool bottom)
{
    auto* dl = ImGui::GetForegroundDrawList();

    float render_width = (float)Render.getWidth();
    float render_height = (float)Render.getHeight();

    ImVec2 pos = ImVec2(screen_cord.x, screen_cord.y);

    if (bottom) {
        pos.x -= (radius + thickness * 0.5f);
        pos.y  = render_height - pos.y;
        pos.y -= (radius + thickness * 0.5f);
    }

    ImVec2 size(radius * 2, radius * 2);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));


    int num_segments = 60;
    int start = static_cast<int>(60 - (value / value_max) * 60);

    const float a_min = -(float)M_PI_2 + (float)IM_PI * 2.0f * ((float)start) / (float)num_segments;
    const float a_max = -(float)M_PI_2 + (float)IM_PI * 2.0f * ((float)num_segments) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius);

    // Render

    dl->AddCircle(centre, radius + 0.5f, 0xFF000000, num_segments, thickness + 1.5f);

    dl->PathClear();

    for (int i = 0; i <= num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        dl->PathLineTo(ImVec2(centre.x + ImCos(a) * radius,
            centre.y + ImSin(a) * radius));
    }

    dl->PathStroke(ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), false, thickness);
}

void drawText3D(const VEC3& world_coord, VEC4 color, const char* text, float font_size ) {
  const CCamera& camera = getActiveCamera();
  const MAT44& view_proj = camera.getViewProjection();

  // Because it divides by w
  // It's in the range -1..1
  VEC3 homo_pos = VEC3::Transform(world_coord, view_proj);

  if (homo_pos.z <= 0.0f || homo_pos.z >= 1.0f )
    return;

  float render_width = (float)Render.getWidth();
  float render_height = (float)Render.getHeight();

  // Convert -1..1 to 0..1024
  float sx = (homo_pos.x + 1.f) * 0.5f * render_width;
  float sy = (1.0f - homo_pos.y) * 0.5f * render_height;

  ImVec2 sz = ImGui::CalcTextSize(text);
  int margin = 4;

  auto* dl = ImGui::GetBackgroundDrawList();
  // center
  sx -= sz.x / 2 - margin;
  sy -= sz.y / 2 - margin;

  if(!font_size)
    dl->AddRectFilled(ImVec2(sx - margin, sy - margin), ImVec2(sx + sz.x + margin, sy + sz.y + margin), 0xa0000000, 2.f);
  
  dl->AddText(NULL, font_size, ImVec2(sx, sy), ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), text);
}

void drawCircle3D(const VEC3& world_coord, VEC4 color, float radius, int num_segments) {

    const CCamera& camera = getActiveCamera();
    const MAT44& view_proj = camera.getViewProjection();

    // Because it divides by w
    // It's in the range -1..1
    VEC3 homo_pos = VEC3::Transform(world_coord, view_proj);

    if (homo_pos.z <= 0.0f || homo_pos.z >= 1.0f)
        return;

    float render_width = (float)Render.getWidth();
    float render_height = (float)Render.getHeight();

    // Convert -1..1 to 0..1024
    float sx = (homo_pos.x + 1.f) * 0.5f * render_width;
    float sy = (1.0f - homo_pos.y) * 0.5f * render_height;

    auto* dl = ImGui::GetBackgroundDrawList();

    dl->AddCircleFilled(ImVec2(sx, sy), radius, ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), num_segments);
}


void drawProgressBar3D(const VEC3& world_coord, VEC4 color, float value, float value_max, VEC2 offset2d) {
    const CCamera& camera = getActiveCamera();
    const MAT44& view_proj = camera.getViewProjection();

    // Because it divides by w
    // It's in the range -1..1
    VEC3 homo_pos = VEC3::Transform(world_coord, view_proj);

    if (homo_pos.z <= 0.0f || homo_pos.z >= 1.0f)
        return;

    float render_width = (float)Render.getWidth();
    float render_height = (float)Render.getHeight();

    // Convert -1..1 to 0..1024
    float sx = (homo_pos.x + 1.f) * 0.5f * render_width;
    float sy = (1.0f - homo_pos.y) * 0.5f * render_height + 25;

    sx += offset2d.x;
    sy += offset2d.y;

    auto* dl = ImGui::GetForegroundDrawList();

    float bkg_rs_w = 30;
    float bkg_rs_h = 5;
    float front_rs_w = bkg_rs_w - 1;
    float front_rs_h = bkg_rs_h - 1;
    float bar_size = mapToRange(0.0f, value_max, 0.0f, bkg_rs_w, value);

    dl->AddRectFilled(ImVec2(sx - bkg_rs_w, sy - bkg_rs_h), ImVec2(sx + bkg_rs_w, sy + bkg_rs_h), 0xf0000000, 2.f);
    if (bar_size) {
        dl->AddRectFilled(ImVec2(sx - front_rs_w, sy - front_rs_h), ImVec2(sx - front_rs_w + 2.0f * bar_size, sy + front_rs_h), ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), 2.f);
    }
}

void drawProgressBar2D(const VEC2& screen_cord, VEC4 color, float value, float value_max, const char* string, float width, 
    float height, bool bottom, float lerp_value, Color lerp_color)
{
    float sx = screen_cord.x;
    float sy = screen_cord.y;

    float render_width = (float)Render.getWidth();
    float render_height = (float)Render.getHeight();

    auto* dl = ImGui::GetForegroundDrawList();

    float bkg_rs_w = width;
    float bkg_rs_h = height;
    float front_rs_w = bkg_rs_w - 4;
    float front_rs_h = bkg_rs_h - 4;
    float bar_size = mapToRange(0.0f, value_max, 0.0f, front_rs_w, value);

    if (bottom) {
        sy = render_height - screen_cord.y;
        sy -= (bkg_rs_h) * 0.5f;
    }

    // Background border
    ImU32 bk_color = 0xf0000000;
    if(value == value_max && width < 120)
        bk_color = 0xf0ff0606;
    dl->AddRectFilled(ImVec2(sx, sy - bkg_rs_h * 0.5f), ImVec2(sx + bkg_rs_w, sy + bkg_rs_h * 0.5f), bk_color, 4.f);
    
    float margin = 2.f;
    sx += margin;

    // Filled background
    dl->AddRectFilled(ImVec2(sx, sy - front_rs_h * 0.5f), ImVec2(sx + front_rs_w, sy + front_rs_h * 0.5f), 0xff404040, 2.f);

    if (lerp_value != -1.f) {
        // add lerp Filled background
        float lerp_bar_size = mapToRange(0.0f, value_max, 0.0f, front_rs_w, lerp_value);
        dl->AddRectFilled(ImVec2(sx, sy - front_rs_h * 0.5f), ImVec2(sx + lerp_bar_size, sy + front_rs_h * 0.5f),
            ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&lerp_color.x), 2.f);
    }

    if (bar_size) {
        dl->AddRectFilled(ImVec2(sx, sy - front_rs_h * 0.5f), ImVec2(sx + bar_size, sy + front_rs_h * 0.5f),
            ImGui::ColorConvertFloat4ToU32(*(ImVec4*)&color.x), 2.f);
    }

    if (string)
    {
        ImVec2 sz = ImGui::CalcTextSize(string);
        float margin_right = 5.f;
        VEC2 text_pos = screen_cord + VEC2(bkg_rs_w - sz.x - margin_right, 0.f);
        Color c = Colors::White;

        if (sx + bar_size > (text_pos.x + sz.x/2.f))
            c = Colors::Black;

        drawText2D(text_pos, c, string, false, true, 0.0f, true);
    }
}
    
void drawCone(CTransform* trans, float cone_fov, float cone_length) 
{
    QUAT qleft = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), cone_fov * 0.5f);
    VEC3 forward = trans->getForward() * cone_length;
    VEC3 e0pos = trans->getPosition();
    VEC3 e0cone_left = VEC3::Transform(forward, qleft);
    drawLine(e0pos, e0pos + e0cone_left, VEC4(1, 0, 0, 1));
    QUAT qright = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), -cone_fov * 0.5f);
    VEC3 e0cone_right = VEC3::Transform(forward, qright);
    drawLine(e0pos, e0pos + e0cone_right, VEC4(1, 0, 0, 1));
}

void drawCircle(VEC3 center, QUAT rotation, float radius, int sides, VEC4 color)
{
  float angle = (float)M_PI * 2.f / sides;
  float _cos = cos(angle);
  float _sin = sin(angle);
  float x1 = radius, z1 = 0.f, x2, z2;

  for (int i = 0; i < sides; i++)
  {
    x2 = _cos * x1 - _sin * z1;
    z2 = _sin * x1 + _cos * z1;

    CTransform trans;
    trans.setRotation(rotation);
    trans.setPosition(center);

    VEC3 startPoint = VEC3(x1, 0, z1);
    VEC3 endPoint = VEC3(x2, 0, z2);

    drawLine(VEC3::Transform(startPoint, trans.asMatrix()), VEC3::Transform(endPoint, trans.asMatrix()), color);
   
    x1 = x2;
    z1 = z2;
  }

}