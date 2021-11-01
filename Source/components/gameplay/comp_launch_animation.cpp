#include "mcv_platform.h"
#include "engine.h"
#include "handle/handle.h"
#include "modules/module_physics.h"
#include "modules/subvert/module_player_interaction.h"
#include "comp_launch_animation.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "components/common/comp_collider.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/messages.h"
#include "utils/resource_json.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("anim_launcher", TCompLaunchAnimation)

void TCompLaunchAnimation::load(const json& j, TEntityParseContext& ctx)
{
	animation_variable = j.value("var", std::string());
	assert(animation_variable.length());

	const CJson* launchers = Resources.get("data/animations/launchers.json")->as<CJson>();
	const json& jData = launchers->getJson();
	assert(jData.count(animation_variable));
	const json& jItem = jData[animation_variable];

	acceptance_dist = jItem.value("acceptance_dist", 0.f);
	offset = loadVEC3(jItem, "offset");

	assert(jItem.count("transform"));
	finalPose.fromJson(jItem["transform"]);
}

void TCompLaunchAnimation::update(float dt)
{
	if (!enabled)
		return;

	timer += dt;

	CEntity* player = getEntityByName("player");
	if (player) {
		player->setPosition(damp<VEC3>(player->getPosition(), finalPose.getPosition(), 2.5f, dt), true);
		TCompTransform* t = player->get<TCompTransform>();
		t->setRotation(dampQUAT(t->getRotation(), finalPose.getRotation(), 2.5f, dt));
	}

	if (timer > lerp_time)
	{
		enabled = false;
		timer = 0.f;
	}
}


void TCompLaunchAnimation::debugInMenu()
{
	ImGui::DragFloat("Acc. Distance", &acceptance_dist, 0.01f, 0.1f, 10.f);
}

void TCompLaunchAnimation::renderDebug()
{
	TCompTransform* transform = get<TCompTransform>();
	drawText3D(transform->getPosition() + VEC3::Up, Colors::Orange, animation_variable.c_str());
}

bool TCompLaunchAnimation::resolve()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return false;

	TCompTransform* transform = get<TCompTransform>();
	TCompTransform* player_transform = player->get<TCompTransform>();

	VHandles colliders;
	bool in_front = EnginePhysics.raycast(player_transform->getPosition(),
		player_transform->getForward(), acceptance_dist, colliders, CModulePhysics::FilterGroup::Interactable);

	in_front &= player_transform->getForward().Dot(transform->getForward()) < 0.f;

	VEC3 pos = transform->getPosition() + offset;
	float dist = VEC3::Distance(pos, player_transform->getPosition());
	return in_front && dist < acceptance_dist;
}

void TCompLaunchAnimation::launch()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return;

	enabled = true;

	TCompPlayerController* c_controller = player->get<TCompPlayerController>();
	c_controller->blendCamera("camera_interact", 3.0f, &interpolators::cubicInOutInterpolator);
	c_controller->setVariable(animation_variable, true);

	PlayerInteraction.setAnimationLauncher(CHandle(this));

	// disable collisions with the player
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setGroupAndMask("interactable", "all_but_player");

	TCompTransform* trans = get<TCompTransform>();
	VEC3 new_pos = trans->getPosition() + trans->getForward() * 0.001f;
	trans->setPosition(new_pos);
	c_collider->setGlobalPose(trans->getPosition(), trans->getRotation());
}

void TCompLaunchAnimation::oncomplete()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return;

	PlayerInteraction.setAnimationLauncher(CHandle());
	PlayerInteraction.setPlaying(false);

	// to avoid Eon exiting the energy wall using time reversal
	TCompTimeReversal* c_time_reversal = player->get<TCompTimeReversal>();
	c_time_reversal->clearBuffer();

	// enable collisions
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setGroupAndMask("interactable", "all");

	TCompPlayerController* c_controller = player->get<TCompPlayerController>();
	c_controller->blendCamera("camera_follow", 1.2f, &interpolators::cubicInOutInterpolator);
}