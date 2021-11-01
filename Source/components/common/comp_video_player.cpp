#include "mcv_platform.h"
#include "comp_video_player.h"
#include "comp_render.h"

DECL_OBJ_MANAGER("video_player", TCompVideoPlayer);

void TCompVideoPlayer::update( float dt ) {
  if( !video )
    return;
  PROFILE_FUNCTION("Video");
  bool finished = video->hasFinished();
  if (video->update(dt)) {
    if (!finished) {
      dbg( "Video has finished!\n");
    }
  }
}

void TCompVideoPlayer::setSource(const std::string& src_name) {
  if( video )
    delete video;
  video = new VideoTexture();
  bool is_ok = video->create(src_name.c_str());
  video->setAutoLoop(auto_loops);
  assert(is_ok);

  CTexture* texture = video->getTexture();
  assert( texture );

  CMaterial* new_material = nullptr;

  // Update My comp render and assign the new material 
  TCompRender* c_render = get<TCompRender>();
  for (auto& dc : c_render->draw_calls) {
    // Create a new material that uses as albedo the new video texture
    // but uses the original pipeline and other textures
    if (dc.material->getName() == material_to_change) {
      if (!new_material) {
        new_material = new CMaterial( *dc.material );
        new_material->setAlbedo(texture);
      }
      dc.material = new_material;
    }
  }
  c_render->updateRenderManager();
}

void TCompVideoPlayer::load(const json& j, TEntityParseContext& ctx) {
  auto_loops = j.value("auto_loops", auto_loops );
  material_to_change = j.value("material_to_change", "data/materials/video.mat");
  if( j.count("src") )
    setSource( j["src"] );
}

void TCompVideoPlayer::debugInMenu() {
  if (!video) {
    ImGui::Text("No video source assigned");
    return;
  }

  ImGui::Text("Aspect Ratio: %f", video->getAspectRatio());
  ImGui::Text("Material: %s", material_to_change.c_str());

  if (ImGui::SmallButton("Pause"))
    video->pause();
  if (ImGui::SmallButton("Resume"))
    video->resume();
}



