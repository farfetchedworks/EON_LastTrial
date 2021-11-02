#include "mcv_platform.h"
#include "comp_attached_to_bone.h"
#include "comp_skeleton.h"
#include "components/common/comp_transform.h"
#include "entity/entity_parser.h"

DECL_OBJ_MANAGER("attached_to_bone", TCompAttachedToBone);

void TCompAttachedToBone::load(const json& j, TEntityParseContext& ctx) {
	bone_name = j["bone"];
	target_entity_name = j["target"];
	skip_rotation = j.value("skip_rotation", skip_rotation);
	offset = loadVEC3(j, "offset");
	offset_rotation = loadVEC3(j, "offset_rotation");
	CHandle h_entity_parent = ctx.findEntityByName(target_entity_name);
	bool is_ok = resolveTarget(h_entity_parent);
	assert(is_ok && "Can't resolve bone");
}

void TCompAttachedToBone::onEntityCreated()
{
	TCompTransform* c_trans = get<TCompTransform>();

	local_t.setPosition(c_trans->getPosition());
	local_t.setScale(c_trans->getScale());
	local_t.setRotation(c_trans->getRotation());

	h_my_transform = c_trans;
}

bool TCompAttachedToBone::resolveTarget(CHandle h_entity_parent)
{
	// Unassign current values
	bone_id = -1;
	h_target_skeleton = CHandle();

	// An handle to entity has been given, confirm it's valid and get the TCompSkeleton handle
	CEntity* e_parent = h_entity_parent;
	if (!e_parent)
		return false;

	// Get the handle of the TCompSkeleton
	h_target_skeleton = e_parent->get<TCompSkeleton>();
	TCompSkeleton* c_skel = h_target_skeleton;
	assert(c_skel);

	// Resolve bone_name to bone_id
	bone_id = c_skel->getBoneIdByName(bone_name);
	return bone_id != -1;
}

void TCompAttachedToBone::update(float delta) {

	if (bone_id == -1 || !enabled)
		return;
	TCompSkeleton* c_skel = h_target_skeleton;
	if (!c_skel)
		return;
	TCompTransform* c_trans = h_my_transform;
	assert(c_trans);

	CTransform t = c_skel->getWorldTransformOfBone(bone_id);
	t = t.combinedWith(local_t);

	CTransform t2;
	t2.fromMatrix(MAT44::CreateTranslation(offset) *
		MAT44::CreateRotationX(offset_rotation.x) *
		MAT44::CreateRotationY(offset_rotation.y) *
		MAT44::CreateRotationZ(offset_rotation.z));
	t = t.combinedWith(t2);

	if (skip_rotation)
		t.setRotation(QUAT::Identity);

	c_trans->set(t);
}

void TCompAttachedToBone::applyOffset(CTransform& trans, VEC3 new_offset)
{
	if (bone_id == -1)
		return;

	TCompSkeleton* c_skel = h_target_skeleton;
	if (!c_skel)
		return;
	TCompTransform* c_trans = h_my_transform;

	assert(c_trans);

	CTransform t = c_skel->getWorldTransformOfBone(bone_id);
	t = t.combinedWith(local_t);

	CTransform t2;
	t2.fromMatrix(MAT44::CreateTranslation(new_offset) *
		MAT44::CreateRotationX(offset_rotation.x) *
		MAT44::CreateRotationY(offset_rotation.y) *
		MAT44::CreateRotationZ(offset_rotation.z));
	t = t.combinedWith(t2);

	// trans = *c_trans;
	trans.fromMatrix(t);
}

void TCompAttachedToBone::debugInMenu() {
	ImGui::Text("Bone ID(%s) = %d", bone_name.c_str(), bone_id);
	ImGui::DragFloat3("Offset", &offset.x, 0.01f, -1.5f, 1.5f);
	ImGui::DragFloat3("Offset Rot", &offset_rotation.x, 0.01f, (float)-M_PI, (float)M_PI);
	local_t.renderInMenu();
}