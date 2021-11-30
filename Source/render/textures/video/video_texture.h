#pragma once

#include "render/render.h"

struct VideoTexture {

  static bool createAPI();
  static void destroyAPI();

  struct InternalData;
  InternalData* internal_data = nullptr;

  bool create(const char* filename);
  void destroy();
  bool update(float dt);

  void pause();
  void resume();
  bool reset();
  void setAutoLoop(bool how);
  bool hasFinished();
  CTexture* getTexture();
  float getAspectRatio() const;
};

