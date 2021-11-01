#include "mcv_platform.h"
#include "cal3d/cal3d.h"
#include "comp_bone_modifier.h"
#include "comp_skeleton.h"
#include "components/common/comp_transform.h"
#include "entity/entity_parser.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("bone_modifier", TCompBoneModifier);

void TCompBoneModifier::load(const json& j, TEntityParseContext& ctx)
{
	bone_name = j["bone"];
	scale = j.value("scale", scale);
	frequency = j.value("frequency", frequency);
}

void TCompBoneModifier::onEntityCreated()
{
	h_skeleton = get<TCompSkeleton>();
	assert(resolveTarget());
}

bool TCompBoneModifier::resolveTarget()
{
	// Unassign current values
	bone_id = -1;
	h_skeleton = CHandle();

	// Get the handle of the TCompSkeleton
	h_skeleton = get<TCompSkeleton>();
	TCompSkeleton* c_skel = h_skeleton;
	assert(c_skel);

	// Resolve bone_name to bone_id
	bone_id = c_skel->getBoneIdByName(bone_name);
	return bone_id != -1;
}

void TCompBoneModifier::updateBone()
{
	if (bone_id == -1)
		return;

	TCompSkeleton* c_skel = h_skeleton;
	if (!c_skel)
		return;

	CalSkeleton* skel = c_skel->model->getSkeleton();
	auto& cal_bones = skel->getVectorBone();

	CalBone* bone = cal_bones[bone_id];
	float f = 1 + sinf((float)Time.current * frequency) * scale;

	c_skel->cb_bones.bones[bone_id] *= MAT44::CreateScale(f) * 
		MAT44::CreateTranslation(-Cal2DX(bone->getTranslation() * (f - 1.f)));
}
