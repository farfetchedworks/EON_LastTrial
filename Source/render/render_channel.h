#pragma once

enum eRenderChannel {
  RC_UNKNOWN,
  RC_SOLID,
  RC_SOLID_STENCIL,
  RC_SHADOW_CASTERS,
  RC_SHADOW_CASTERS_DYNAMIC,
  RC_EMISSIVE,
  RC_DISTORSIONS,
  RC_WATER,
  RC_BLACK_HOLES,
  RC_DECALS,
  RC_TRANSPARENT,
  RC_UI,
  RC_DEBUG,
  RC_COUNT,
};

extern NamedValues<eRenderChannel> render_channel_names;
