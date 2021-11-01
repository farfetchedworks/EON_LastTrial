#include "mcv_platform.h"
#include "engine.h"
#include "comp_lock_target.h"
#include "components/common/comp_transform.h"
#include "skeleton/comp_skeleton.h"

DECL_OBJ_MANAGER("lock_target", TCompLockTarget);

void TCompLockTarget::load(const json& j, TEntityParseContext& ctx)
{
	bone_name = j["bone"];
	height = j.value("height", height);
}

void TCompLockTarget::debugInMenu()
{
	ImGui::DragFloat("Height", &height, 0.01f, 0.f, 5.f);
}

void TCompLockTarget::onEntityCreated()
{
	bool is_ok = resolveTarget();
	assert(is_ok);
}

bool TCompLockTarget::resolveTarget()
{
	bone_id = -1;
	h_target_skeleton = CHandle();

	CEntity* e_parent = CHandle(this).getOwner();
	assert(e_parent);

	// Get the handle of the TCompSkeleton
	h_target_skeleton = e_parent->get<TCompSkeleton>();
	TCompSkeleton* c_skel = h_target_skeleton;
	assert(c_skel);

	// Resolve bone_name to bone_id
	bone_id = c_skel->getBoneIdByName(bone_name);
	return bone_id != -1;
}

VEC3 TCompLockTarget::getPosition()
{
	TCompSkeleton* c_skel = h_target_skeleton;
	CTransform tBone = c_skel->getWorldTransformOfBone(bone_id);
	return tBone.getPosition();
}