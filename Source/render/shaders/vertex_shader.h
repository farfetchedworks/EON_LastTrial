#pragma once

class CVertexShader {

  ID3D11VertexShader*       vs = nullptr;
  ID3D11InputLayout*        vertex_layout = nullptr;
  const CVertexDeclaration* vertex_decl = nullptr;

public:

  bool create(const char* filename, const char* entry_point, const char* new_vertex_decl);
  void destroy();

  void activate() const;
  static void deactivateResources();
};


