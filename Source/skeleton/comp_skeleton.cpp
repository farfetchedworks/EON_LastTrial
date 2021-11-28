#include "mcv_platform.h"
#include "comp_skeleton.h"
#include "engine.h"
#include "game_core_skeleton.h"
#include "render/draw_primitives.h"
#include "modules/module_physics.h"
#include "render/render_module.h"
#include "comp_bone_modifier.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/controllers/pawn_utils.h"
#include "components/controllers/comp_pawn_controller.h"
#include "components/common/comp_aabb.h"
#include "components/common/comp_culling.h"
#include "components/ai/comp_bt.h"
#include "components/common/comp_fsm.h"
#include "comp_skeleton_ik.h"
#include "comp_skel_lookat.h"

// Changed name from skeleton to force to be parsed before the comp_mesh
DECL_OBJ_MANAGER("armature", TCompSkeleton);

// Not necessary if we use multithreading
const bool OPTIMIZE_UPDATES = true;

// ---------------------------------------------------------------------------------------
// Cal2DX conversions, VEC3 are the same, QUAT must change the sign of w
CalVector DX2Cal(VEC3 p) {
	return CalVector(p.x, p.y, p.z);
}
CalQuaternion DX2Cal(QUAT q) {
	return CalQuaternion(q.x, q.y, q.z, -q.w);
}
VEC3 Cal2DX(CalVector p) {
	return VEC3(p.x, p.y, p.z);
}
QUAT Cal2DX(CalQuaternion q) {
	return QUAT(q.x, q.y, q.z, -q.w);
}
MAT44 Cal2DX(CalVector trans, CalQuaternion rot) {
	return
		MAT44::CreateFromQuaternion(Cal2DX(rot))
		* MAT44::CreateTranslation(Cal2DX(trans))
		;
}

TCompSkeleton::TCompSkeleton() {
  model = nullptr;
}

TCompSkeleton::~TCompSkeleton() {
  cb_bones.destroy();
  if (model)
    delete model;
  model = nullptr;
}

void TCompSkeleton::onEntityCreated() {
  bool is_ok = cb_bones.create(CB_SLOT_SKIN, "SkinBones");
  assert(is_ok);
}

void TCompSkeleton::load(const json& j, TEntityParseContext& ctx) {
	std::string src = j["src"];
	auto core = Resources.get(src)->as<CGameCoreSkeleton>();
	stop_at = j.value("stop_at", stop_at);
	paused = j.value("paused", paused);
	
	// Cal3d
	model = new CalModel((CalCoreModel*)core);
	model->setUserData(CHandle(this).getOwner().asVoidPtr());

	updateAABB();

	model->getMixer()->blendCycle(0, 1.0f, 0.0f);
	model->update(0.f);
}

void TCompSkeleton::resume(float stops) {
	paused = false;
	stop_at = stops;
}

bool TCompSkeleton::managePauses(float dt) {
	
	if (!paused && stop_at > 0.f) {

		if (model->getMixer()->getAnimationTime() > stop_at)
		{
			paused = true;
		}
	}

	return paused;
}

void TCompSkeleton::updateLookAt(float delta) {
	PROFILE_FUNCTION("updatePostSkeleton");
	TCompSkelLookAt* lookat = get< TCompSkelLookAt>();
	if (lookat)
		lookat->update(delta);
}

void TCompSkeleton::updateIKs(float delta) {
	PROFILE_FUNCTION("updateIKs");
	TCompSkeletonIK* c_iks = get<TCompSkeletonIK>();
	if (c_iks)
		c_iks->update(delta);
}

void TCompSkeleton::updatePreMultithread(float dt)
{
	if(managePauses(dt))
		return;

	TCompTransform* c_trans = get<TCompTransform>();
	if (!c_trans) return;

	TCompPawnController* controller = get<TCompPawnController>();

	if (controller) {
		dt *= controller->speed_multiplier;
	}

	PROFILE_FUNCTION("Cal3d Update Animation");

	model->getMixer()->setWorldTransform(DX2Cal(c_trans->getPosition()), DX2Cal(c_trans->getRotation()));
	model->updateAnimation(dt);
}

void TCompSkeleton::update(float dt) {

	auto om = getObjectManager<TCompSkeleton>();

	if (!om->multithreaded) {

		if (managePauses(dt))
			return;

		updatePreMultithread(dt);
	}
	else if (paused)
		return;

	PROFILE_FUNCTION("Skeleton Update");

	TCompTransform* c_trans = get<TCompTransform>();
	if (!c_trans) return;

	CEntity* player = getEntityByName("player");
	CEntity* owner = getEntity();
	TCompCollider* collider = get<TCompCollider>();
	TCompFSM* c_fsm = get<TCompFSM>();
	CEntity* e_camera = EngineRender.getActiveCamera();

	has_to_update = true;

	if (OPTIMIZE_UPDATES)
	{
		if (e_camera && owner != player) {
			CHandle h_aabb = get<TCompAbsAABB>();
			const TCompCulling* c_culling = e_camera->get<TCompCulling>();
			const TCompCulling::TCullingBits* culling_bits = &c_culling->bits;

			if (h_aabb.isValid() && culling_bits) {
				const uint32_t idx = h_aabb.getExternalIndex();
				has_to_update &= culling_bits->test(idx);
			}
		}

		// Optimize a little bit by not updating the skeleton
		{
			// Disable if too far away	(50 m?)

			// To avoid issues when using the preview mode
			if (player) {
				has_to_update &= (owner->getPosition() - player->getPosition()).Length() < 50.f;
			}

			// If the skeleton is moving, we have to update the skeleton anyways 
			TCompBT* my_bt = get<TCompBT>();
			if (my_bt) {
				has_to_update |= (collider->getLinearVelocity().Length() > 0.001f);
			}
		}

		if (!first_update) {
			has_to_update = first_update = true;
		}

		if (c_fsm) {
			c_fsm->setHasToUpdate(has_to_update);
		}
	}
	
	TCompPawnController* controller = get<TCompPawnController>();

	if (controller) {
		dt *= controller->speed_multiplier;
	}

	if(has_to_update){
		PROFILE_FUNCTION("Cal3d Skeleton");
		model->updateSkeleton(dt);
	}

	updateLookAt(dt);
	updateIKs(dt);
	updateAABB();

	// Moved to post multithread:
	// - updateCteBones();
	// - root motion 

	if (!om->multithreaded) {
		updatePostMultithread(dt);
	}
}

void TCompSkeleton::updatePostMultithread(float dt) {

	if (!has_to_update || paused)
		return;

	updateCteBones();

	TCompTransform* c_trans = get<TCompTransform>();
	if (!c_trans) return;

	TCompCollider* collider = get<TCompCollider>();
	TCompPawnController* controller = get<TCompPawnController>();

	// Modulate time
	{
		if (controller) {
			dt *= controller->speed_multiplier;
		}
	}

	PROFILE_FUNCTION("Motion");

	// Transfer yaw rotation back to the entity
	float delta_yaw = model->getMixer()->getAndClearDeltaYaw();
	if (fabsf(delta_yaw) > 0.f) {
		QUAT q = QUAT::CreateFromYawPitchRoll(delta_yaw, 0, 0);
		c_trans->setRotation(c_trans->getRotation() * q);
	}

	// Transfer the root motion back to the entity
	CalVector delta_pos = model->getMixer()->getAndClearDeltaLocalRootMotion();

	VEC3 root_motion_delta_local = Cal2DX(delta_pos);
	VEC3 root_motion_delta_abs = VEC3::Transform(root_motion_delta_local, c_trans->getRotation());

	if (root_motion_delta_abs.Length() > 0.f)
	{
		if (!collider || !collider->is_controller)
		{
			// Use root_motion_delta_abs to collide with physics for example
			c_trans->setPosition(c_trans->getPosition() + root_motion_delta_abs);
		}
		else
		{
			float g = -9.81f;
			if (controller->isPlayerDashing())
				g = -1.f;

			PawnUtils::movePhysics(collider, root_motion_delta_abs, dt, g);
		}
	}
}

void TCompSkeleton::renderDebug() {
	int num_bones = (int)model->getSkeleton()->getCoreSkeleton()->getVectorCoreBone().size();
	std::vector<VEC3> lines;
	lines.resize(num_bones * 2);
	model->getSkeleton()->getBoneLines(&lines.data()->x);
	for (int i = 0; i < num_bones; ++i) {
		drawLine(lines[i * 2], lines[i * 2 + 1], Colors::White);
	}

	if (show_bone_axis) {
		for (int i = 0; i < num_bones; ++i) {
			CTransform transform = getWorldTransformOfBone(i);
			transform.setScale(VEC3(show_bone_axis_scale));
			drawAxis(transform.asMatrix());
		}
	}

	// Show list of bones
	//auto mesh = Resources.get("axis.mesh")->as<CMesh>();
	auto core = (CGameCoreSkeleton*)model->getCoreModel();
	auto& bones_to_debug = core->bone_ids_to_debug;
	for (auto it : bones_to_debug) {
		CalBone* cal_bone = model->getSkeleton()->getBone(it);
		CalCoreBone* cb = cal_bone->getCoreBone();
		assert(cb);
		QUAT rot = Cal2DX(cal_bone->getRotationAbsolute());
		VEC3 pos = Cal2DX(cal_bone->getTranslationAbsolute());
		MAT44 mtx;
		mtx = MAT44::CreateFromQuaternion(rot) * MAT44::CreateTranslation(pos);
		drawAxis(mtx);
		drawText3D(pos, Colors::ShinyBlue, cb->getName().c_str());
	}

}

void TCompSkeleton::updateCteBones()
{
	if (custom_bone_update) return;

	PROFILE_FUNCTION("updateCteBones");
	// Pointer to the first float of the array of matrices
	 //float* fout = &cb_bones.bones[0]._11;
	float* fout = &cb_bones.bones[0]._11;

	assert(model);
	CalSkeleton* skel = model->getSkeleton();
	auto& cal_bones = skel->getVectorBone();
	assert(cal_bones.size() < MAX_SUPPORTED_BONES);

	// For each bone from the cal model
	for (size_t bone_idx = 0; bone_idx < cal_bones.size(); ++bone_idx)
	{
		CalBone* bone = cal_bones[bone_idx];

		const CalMatrix& cal_mtx = bone->getTransformMatrix();
		const CalVector& cal_pos = bone->getTranslationBoneSpace();

		*fout++ = cal_mtx.dxdx;
		*fout++ = cal_mtx.dydx;
		*fout++ = cal_mtx.dzdx;
		*fout++ = 0.f;
		*fout++ = cal_mtx.dxdy;
		*fout++ = cal_mtx.dydy;
		*fout++ = cal_mtx.dzdy;
		*fout++ = 0.f;
		*fout++ = cal_mtx.dxdz;
		*fout++ = cal_mtx.dydz;
		*fout++ = cal_mtx.dzdz;
		*fout++ = 0.f;
		*fout++ = cal_pos.x;
		*fout++ = cal_pos.y;
		*fout++ = cal_pos.z;
		*fout++ = 1.f;
	}

	TCompBoneModifier* bModifier = get<TCompBoneModifier>();

	if (bModifier)
	{
		bModifier->updateBone();
	}

	cb_bones.updateFromCPU();
}

CTransform TCompSkeleton::getWorldTransformOfBone(const std::string& bone_name, bool correct_mcv) const
{
	int bone_id = getBoneIdByName(bone_name);
	assert(bone_id != -1);
	return getWorldTransformOfBone(bone_id, correct_mcv);
}

CTransform TCompSkeleton::getWorldTransformOfBone(int bone_id, bool correct_mcv) const
{
	assert(bone_id >= 0);
	assert(bone_id < model->getCoreModel()->getCoreSkeleton()->getVectorCoreBone().size());

	// Calculate absolute translation and rotation from bone space data
	// Check CalBone::calculateState to understand the maths
	CalBone* bone = model->getSkeleton()->getBone(bone_id);
	MAT44 trans = cb_bones.bones[bone_id];

	// cb_bones has bone space data
	VEC3 scale;
	VEC3 bone_space_trans = trans.Translation();
	QUAT bone_space_rot = getQUATfromMatrix(trans);

	// Extract absolute rotation
	QUAT core_rot = Cal2DX(bone->getCoreBone()->getRotationBoneSpace());
	QUAT core_rot_inv;
	core_rot.Inverse(core_rot_inv);
	QUAT absolute_rot = core_rot_inv * bone_space_rot;

	// Extract absolute translation
	CalVector core_trans = bone->getCoreBone()->getTranslationBoneSpace();
	core_trans *= DX2Cal(absolute_rot);
	VEC3 absolute_trans = bone_space_trans - Cal2DX(core_trans);

	if (correct_mcv) {
		QUAT q = QUAT::CreateFromRotationMatrix(MAT44::CreateRotationX(deg2rad(-90.f)));
		q *= QUAT::CreateFromRotationMatrix(MAT44::CreateRotationY(deg2rad(-90.f)));
		absolute_rot = QUAT::Concatenate(absolute_rot, q);
	}

	CTransform transform;
	transform.setPosition(absolute_trans);
	transform.setRotation(absolute_rot);

	return transform;
}

int TCompSkeleton::getBoneIdByName(const std::string& bone_name) const {
	return model->getCoreModel()->getCoreSkeleton()->getCoreBoneId(bone_name);
}

const CGameCoreSkeleton* TCompSkeleton::getGameCore() const {
	return (CGameCoreSkeleton*)model->getCoreModel();
}

void TCompSkeleton::updateAABB() {
  PROFILE_FUNCTION("updateAABB");
  TCompAbsAABB* aabb = get<TCompAbsAABB>();
  if (!aabb)
    return;

  VEC3 points[MAX_SUPPORTED_BONES];
  CalSkeleton* skel = model->getSkeleton();
  auto& cal_bones = skel->getVectorBone();
  assert(cal_bones.size() < MAX_SUPPORTED_BONES);

  auto& bone_ids_for_aabb = getGameCore()->bone_ids_for_aabb;
  int out_idx = 0;
  for (int bone_idx : bone_ids_for_aabb) {
    CalBone* bone = cal_bones[bone_idx];
    points[out_idx] = Cal2DX(bone->getTranslationAbsolute());
    ++out_idx;
  }
  {
    PROFILE_FUNCTION("CreateFromPoints");
    AABB::CreateFromPoints(*aabb, out_idx, points, sizeof(VEC3));
  }
  aabb->Extents = getGameCore()->aabb_extra_factor * VEC3(aabb->Extents);
}


void TCompSkeleton::debugInMenu() {

	static float in_delay = 0.3f;
	static float out_delay = 0.3f;
	static int anim_id = 0;
	static bool auto_lock = false;
	static bool root_motion = false;
	static bool root_yaw = false;

	// Play actions/cycle from the menu
	const char* options[64] = {};
	for (int i = 0; i < model->getCoreModel()->getCoreAnimationCount(); ++i) {
		options[i] = model->getCoreModel()->getCoreAnimation(i)->getName().c_str();
	}

	ImGui::Combo("Anim", &anim_id, options, model->getCoreModel()->getCoreAnimationCount());
	auto core_anim = model->getCoreModel()->getCoreAnimation(anim_id);
	ImGui::Text("Fade times");
	ImGui::DragFloat("In", &in_delay, 0.01f, 0, 1.f);
	ImGui::DragFloat("Out", &out_delay, 0.01f, 0, 1.f);
	ImGui::Checkbox("Keep action alive", &auto_lock);
	ImGui::Checkbox("Apply Root Motion", &root_motion);
	ImGui::Checkbox("Apply Root Yaw", &root_yaw);
	if (ImGui::SmallButton("Add as Cycle")) {
		for (auto a : model->getMixer()->getAnimationCycle()) {
			auto core = (CGameCoreSkeleton*)model->getCoreModel();
			int id = core->getCoreAnimationId(a->getCoreAnimation()->getName());
			model->getMixer()->clearCycle(id, out_delay);
		}
		model->getMixer()->blendCycle(anim_id, 1.0f, in_delay);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Add as Action")) {
		model->getMixer()->executeAction(anim_id, in_delay, out_delay, 1.0f, auto_lock, root_motion, root_yaw);
	}

	// Dump Mixer
	auto mixer = model->getMixer();
	for (auto a : mixer->getAnimationActionList()) {
		ImGui::PushID(a);
		ImGui::Text("Action %s S:%d W:%1.2f Time:%1.4f/%1.4f"
			, a->getCoreAnimation()->getName().c_str()
			, a->getState()
			, a->getWeight()
			, a->getTime()
			, a->getCoreAnimation()->getDuration()
		);
		ImGui::SameLine();
		if (ImGui::SmallButton("X")) {
			auto core = (CGameCoreSkeleton*)model->getCoreModel();
			int id = core->getCoreAnimationId(a->getCoreAnimation()->getName());
			if (a->getState() == CalAnimation::State::STATE_STOPPED)
				mixer->removeAction(id, 0.f);
			else
				a->remove(out_delay);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	// ImGui::Text("Mixer Time: %f/%f", mixer->getAnimationTime(), mixer->getAnimationDuration());
	for (auto a : mixer->getAnimationCycle()) {
		ImGui::PushID(a);
		ImGui::Text("Cycle %s S:%d W:%1.2f Time:%1.4f"
			, a->getCoreAnimation()->getName().c_str()
			, a->getState()
			, a->getWeight()
			, a->getCoreAnimation()->getDuration()
		);
		ImGui::SameLine();
		if (ImGui::SmallButton("X")) {
			auto core = (CGameCoreSkeleton*)model->getCoreModel();
			int id = core->getCoreAnimationId(a->getCoreAnimation()->getName());
			mixer->clearCycle(id, out_delay);
		}
		ImGui::PopID();
	}

	// Show Skeleton Resource
	if (ImGui::TreeNode("Core")) {
		auto core_skel = (CGameCoreSkeleton*)model->getCoreModel();
		if (core_skel)
			core_skel->renderInMenu();
		ImGui::TreePop();
	}


	ImGui::Checkbox("Show All Bones", &show_bone_axis);
	if (show_bone_axis)
		ImGui::DragFloat("Axis Scale", &show_bone_axis_scale, 0.02f, 0.f, 1.0f);

}

/*
*   Play animation async (it's state independent)
*/
void TCompSkeleton::playAnimation(const std::string& name, ANIMTYPE type, float blendIn, float blendOut,
	float weight, bool root_motion, bool root_yaw, bool lock)
{
	assert(model);

	if (type == ANIMTYPE::CYCLE)
		playCycle(name, blendIn, weight);
	else
		playAction(name, blendIn, blendOut, weight, root_motion, root_yaw, lock);
}

void TCompSkeleton::playAction(const std::string& name, float blendIn, float blendOut, float weight,
	bool root_motion, bool root_yaw, bool lock)
{
	auto core = (CGameCoreSkeleton*)model->getCoreModel();
	int anim_id = core->getCoreAnimationId(name);
	model->getMixer()->executeAction(anim_id, blendIn, blendOut, weight, lock, root_motion, root_yaw);
}

void TCompSkeleton::playCycle(const std::string& name, float blend, float weight)
{
	for (auto a : model->getMixer()->getAnimationCycle()) {
		auto core = (CGameCoreSkeleton*)model->getCoreModel();
		int id = core->getCoreAnimationId(a->getCoreAnimation()->getName());
		model->getMixer()->clearCycle(id, blend);
	}

	lockBlendState();

	auto core = (CGameCoreSkeleton*)model->getCoreModel();
	int anim_id = core->getCoreAnimationId(name);
	model->getMixer()->blendCycle(anim_id, weight, blend);
}

/*
*   Stop animation async (it's state independent)
*/

void TCompSkeleton::stopAnimation(const std::string& name, ANIMTYPE type, float blendOut)
{
	assert(model);
	auto core = (CGameCoreSkeleton*)model->getCoreModel();
	int anim_id = core->getCoreAnimationId(name);
	auto mixer = model->getMixer();

	if (type == ANIMTYPE::CYCLE) {
		mixer->clearCycle(anim_id, blendOut);
		unlockBlendState();
	}
	else
		mixer->removeAction(anim_id, blendOut);
}