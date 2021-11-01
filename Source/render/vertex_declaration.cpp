#include "mcv_platform.h"
#include "vertex_declaration.h"

// RenderMesh(OnlyPos) | CookedMesh

// Define the input layout
static D3D11_INPUT_ELEMENT_DESC layout_pos_color[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
CVertexDeclaration vtx_decl_pos_color("PosColor", layout_pos_color, ARRAYSIZE(layout_pos_color));

static D3D11_INPUT_ELEMENT_DESC layout_pos[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
CVertexDeclaration vtx_decl_pos("Pos", layout_pos, ARRAYSIZE(layout_pos));

static D3D11_INPUT_ELEMENT_DESC layout_pos_normal_uv_tan[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
CVertexDeclaration vtx_decl_pos_normal_uv_tan("PosNUvTan", layout_pos_normal_uv_tan, ARRAYSIZE(layout_pos_normal_uv_tan));

static D3D11_INPUT_ELEMENT_DESC layout_pos_uv[] =
{
  // Normal PosNUvTan
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
CVertexDeclaration vtx_decl_pos_uv("PosUv", layout_pos_uv, ARRAYSIZE(layout_pos_uv));


static D3D11_INPUT_ELEMENT_DESC layout_pos_normal_uv_tan_skin[] =
{
    // Normal PosNUvTan
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    // Skin info
    { "BONEIDS",  0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "WEIGHTS",  0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
CVertexDeclaration vtx_decl_pos_normal_uv_tan_skin("PosNUvTanSkin", layout_pos_normal_uv_tan_skin, ARRAYSIZE(layout_pos_normal_uv_tan_skin));

CVertexDeclaration::CVertexDeclaration(const char* new_name, const D3D11_INPUT_ELEMENT_DESC* new_layout, UINT new_num_elements)
  : layout( new_layout )
  , num_elements( new_num_elements )
  , name(new_name)
{ }

const CVertexDeclaration* getVertexDeclarationByName(const std::string& name) {
  if (name == vtx_decl_pos_color.name)
    return &vtx_decl_pos_color;
  if (name == vtx_decl_pos.name)
    return &vtx_decl_pos;
  if (name == vtx_decl_pos_uv.name)
    return &vtx_decl_pos_uv;
  if (name == vtx_decl_pos_normal_uv_tan.name)
    return &vtx_decl_pos_normal_uv_tan;
  if (name == vtx_decl_pos_normal_uv_tan_skin.name || name == "PosSkin")
    return &vtx_decl_pos_normal_uv_tan_skin;
  if(name == vtx_decl_pos_uv.name)
    return &vtx_decl_pos_uv;
  fatal("Unknown vertex declaration requested %s\n", name.c_str());
  return nullptr;
}
