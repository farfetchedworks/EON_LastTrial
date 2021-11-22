#include "mcv_platform.h"
#include "comp_skel_lookat.h"
#include "components/common/comp_transform.h"
#include "skeleton/comp_skeleton.h"
#include "cal3d/cal3d.h"
#include "cal3d2engine.h"
#include "entity/entity_parser.h"
#include "render/draw_primitives.h"
#include "game_core_skeleton.h"

DECL_OBJ_MANAGER("skel_lookat", TCompSkelLookAt);

void TCompSkelLookAt::load(const json& j, TEntityParseContext& ctx) {
	if (j.count("target"))
		target = loadVEC3(j["target"]);
	amount = j.value("amount", amount);
	enabled = j.value("enabled", enabled);
	look_at_player = j.value("look_at_player", look_at_player);
	target_transition_factor = j.value("target_transition_factor", target_transition_factor);
	target_entity_name = j.value("target_entity_name", "");
	offset = loadVEC3(j, "offset", VEC3::Zero);
	head_bone_name = j.value("head_bone_name", "");
}

CHandle getPlayer()
{
	CEntity* playerEntity = getEntityByName("EonHolo");

	if (!playerEntity) {
		playerEntity = getEntityByName("player");
	}

	return playerEntity;
}

void TCompSkelLookAt::update(float dt) {

	if (!enabled)
		return;

	TCompSkeleton* c_skel = h_skeleton;

	if (look_at_player) {
		h_target_entity = getPlayer();
	}else if(!h_target_entity.isValid() && !target_entity_name.empty()) {
		h_target_entity = getEntityByName(target_entity_name);
	}

	// If we have a target entity, use it to assign a target position
	CEntity* owner = getEntity();
	CEntity* e = h_target_entity;
	if (e) {
		// make the target smoothly change between positions
		TCompTransform* t = e->get<TCompTransform>();
		VEC3 owner_pos = owner->getPosition();
		new_target = t->getPosition() + offset;
	}

	target = VEC3::Lerp(target, new_target, target_transition_factor);

	if (c_skel == nullptr) {
		// Search the parent entity by name
		CEntity* e_entity = CHandle(this).getOwner();
		if (!e_entity)
			return;
		// The entity I'm tracking should have an skeleton component.
		h_skeleton = e_entity->get<TCompSkeleton>();
		assert(h_skeleton.isValid());
		c_skel = h_skeleton;
	}

	// The cal3d skeleton instance
	CalSkeleton* skel = c_skel->model->getSkeleton();

	// Change amount depending on the orientation
	TCompTransform* t = get<TCompTransform>();
	float delta_yaw = fabsf(t->getYawRotationToAimTo(target)) / (float)M_PI;

	float f_amount = amount;

	if (delta_yaw > 0.6f || stop_looking)
		f_amount = 0.f;

	lerp_amount = damp(lerp_amount, f_amount, 1.f, dt);

	if (lerp_amount == 0.f && stop_looking)
	{
		stop_looking = false;
		enabled = false;
	}

	// The set of bones to correct
	auto core = (CGameCoreSkeleton*)c_skel->getGameCore();
	for (auto& it : core->lookat_corrections)
		it.apply(skel, target, lerp_amount);
}

void TCompSkelLookAt::setEnabled(bool v)
{
	enabled = v;
	stop_looking = !enabled;
}

void TCompSkelLookAt::stopLooking()
{
	stop_looking = true;
}

void TCompSkelLookAt::setTarget(const VEC3& target_pos)
{
	new_target = target_pos + offset;

	// Disable entity options
	look_at_player = false;
	h_target_entity = CHandle();
	target_entity_name = "";
}

bool TCompSkelLookAt::isLookingAtTarget(float accept_dist)
{
	return (target - new_target).Length() < accept_dist;
}

void TCompSkelLookAt::renderDebug() {
	const CMesh* mesh = Resources.get("unit_wired_cube.mesh")->as<CMesh>();
	drawPrimitive(mesh, MAT44::CreateTranslation(target) * MAT44::CreateScale(0.2f), Colors::Red);
}

void TCompSkelLookAt::debugInMenu() {
	ImGui::InputFloat3("Target", &target.x);
	ImGui::LabelText("Target Name", "%s", target_entity_name.c_str());
	ImGui::DragFloat("Amount", &amount, 0.01f, 0.f, 1.0f);
	ImGui::DragFloat("Lerp Amount", &lerp_amount, 0.01f, 0.f, 1.0f);
	ImGui::DragFloat("Transition Factor", &target_transition_factor, 0.01f, 0.f, 1.0f);
}
