#pragma once

class CVertexDeclaration {
public:
  const D3D11_INPUT_ELEMENT_DESC* layout = nullptr;
  UINT                            num_elements = 0;
  std::string                     name;

  CVertexDeclaration(const char* name, const D3D11_INPUT_ELEMENT_DESC* new_layout, UINT new_num_element);
};

const CVertexDeclaration* getVertexDeclarationByName(const std::string& name);
