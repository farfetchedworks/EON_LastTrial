#pragma once

class CPixelShader {

  ID3D11PixelShader* ps = NULL;

public:

  bool create(const char* filename, const char* entry_point);
  void destroy();

  void activate() const;

};


