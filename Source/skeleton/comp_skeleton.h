#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "render/shaders/shader_cte.h"
#include "cal3d/cal3d.h"

class CalModel;
class CGameCoreSkeleton;

CalVector DX2Cal(VEC3 p);
CalQuaternion DX2Cal(QUAT q);
VEC3 Cal2DX(CalVector p);
QUAT Cal2DX(CalQuaternion q);
MAT44 Cal2DX(CalVector trans, CalQuaternion rot);

enum class ANIMTYPE {
	CYCLE = 0,
	ACTION
};

struct TCompSkeleton : public TCompBase {

	DECL_SIBLING_ACCESS();

public:

	CalModel* model = nullptr;
	CShaderCte<CtesSkin> cb_bones;
	bool show_bone_axis = false;
	float show_bone_axis_scale = 0.1f;
	bool custom_bone_update = false;
	bool blendstate_locked = false;

	bool first_update = false;
	bool has_to_update = true;

	bool paused = false;
	float stop_at = -1.f;
	float pause_timer = 0.f;

	TCompSkeleton();
	~TCompSkeleton();

	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void updatePreMultithread(float dt);
	void updateMultithread(float dt) { update(dt); }
	void updatePostMultithread(float dt);
	void renderDebug();
	void debugInMenu();
	void onEntityCreated();
	void updateCteBones();

	void resume(float stop_at = -1.f);
	bool managePauses(float dt);

	void lockBlendState() { blendstate_locked = true; }
	void unlockBlendState() { blendstate_locked = false; }
	bool bsLocked() { return blendstate_locked; }

	void playAction(const std::string& name, float blendIn = 0.25f, float blendOut = 0.25f,
		float weight = 1.f, bool root_motion = false, bool root_yaw = false, bool lock = false);
	void playCycle(const std::string& name, float blend = 0.25f, float weight = 1.f);

	void playAnimation(const std::string& name, ANIMTYPE type, float blendIn = 0.25f, float blendOut = 0.25f,
		float weight = 1.f, bool root_motion = false, bool root_yaw = false, bool lock = false);
	void stopAnimation(const std::string& name, ANIMTYPE type, float blendOut = 0.25f);

	void updateLookAt(float dt);
	void updateAABB();
	void updateIKs(float dt);

	CTransform getWorldTransformOfBone(const std::string& bone_name, bool correct_mcv = false) const;
	CTransform getWorldTransformOfBone(int bone_id, bool correct_mcv = false) const;
	int getBoneIdByName(const std::string& bone_name) const;

	const CGameCoreSkeleton* getGameCore() const;
};

