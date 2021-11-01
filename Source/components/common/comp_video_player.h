#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "render/textures/video/video_texture.h"

struct TCompVideoPlayer : public TCompBase {
public:
  DECL_SIBLING_ACCESS();

  VideoTexture* video = nullptr;
  bool auto_loops = false;
  std::string material_to_change;

  void load(const json& j, TEntityParseContext& ctx);
  void debugInMenu();
  void update( float dt );
  void setSource( const std::string& src_name );
};

