#pragma once

#include "resources/resources.h"
#include "bone_correction.h"
#include "Cal3d/cal3d.h"

// ---------------------------------------------------------------------------------------
// Cal2DX conversions, VEC3 are the same, QUAT must change the sign of w
class CGameCoreSkeleton : public IResource, public CalCoreModel {

	std::string root_path;

	struct TParams {
		float blendIn = -1.f;
		float blendOut = -1.f;
		float animTime = -1.f;
	};

	std::vector<TParams> anim_params;

	void convertCalMeshToEngineMesh(const std::string& mesh_name, int mesh_id);

public:

  float              bone_ids_debug_scale = 1.f;
  std::vector< int > bone_ids_to_debug;
  std::vector< int > bone_ids_for_aabb;
  VEC3               aabb_extra_factor = VEC3::One;
  std::vector< TBoneCorrection > lookat_corrections;

  CGameCoreSkeleton(const std::string& name) : CalCoreModel( name ) {}

  bool create(const std::string& res_name);
  bool renderInMenu() const override;

  TParams getAnimationUserParams(int anim_id);
};


