#include "mcv_platform.h"
#include "engine.h"
#include "modules/module_physics.h"
#include "modules/game/module_player_interaction.h"
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
	if(j.count("value") > 0)
	{
		useVariableValue = true;
		variable_value = j.value("value", variable_value);
	}

	new_group = j.value("group", std::string());
	animation_variable = j.value("var", std::string());
	assert(animation_variable.length());

	const CJson* launchers = Resources.get("data/animations/launchers.json")->as<CJson>();
	const json& jData = launchers->getJson();
	assert(jData.count(animation_variable));
	const json& jItem = jData[animation_variable];

	acceptance_dist = jItem.value("acceptance_dist", acceptance_dist);
	acceptance_angle = jItem.value("acceptance_angle", acceptance_angle);
	skipFront = jItem.value("skip_front", skipFront);
	useInteractCam = jItem.value("use_cam", useInteractCam);
	rotateToFace = jItem.value("rotate_to_face", rotateToFace);
	offset = loadVEC3(jItem, "offset");

	if (jItem.count("use_limits") > 0)
	{
		useLimits = true;
		Xlimits = loadVEC2(jItem, "x_limits");
		Zlimits = loadVEC2(jItem, "z_limits");
	}

	if (jItem.count("transform") > 0)
	{
		useFinalPose = true;
		finalPose.fromJson(jItem["transform"]);
	}
}

void TCompLaunchAnimation::update(float dt)
{
	if (!enabled)
		return;

	if (rotateToFace)
	{
		CEntity* player = getEntityByName("player");
		if (!player)
			return;

		TCompTransform* player_transform = player->get<TCompTransform>();
		player_transform->setRotation(dampQUAT(player_transform->getRotation(), targetRotation, 8.f, dt));
	}

	if (!useFinalPose)
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
	ImGui::DragFloat3("Offset", &offset.x, 0.01f, -10.f, 10.f);
	ImGui::DragFloat2("X limits", &Xlimits.x, 0.01f, -10.f, 10.f);
	ImGui::DragFloat2("Z limits", &Zlimits.x, 0.01f, -10.f, 10.f);
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

	// target is in front?
	VHandles colliders;
	physx::PxU32 mask = CModulePhysics::FilterGroup::Interactable | CModulePhysics::FilterGroup::InteractableNoCamCollision;
	bool is_ok = EnginePhysics.raycast(player_transform->getPosition(),
		player_transform->getForward(), 1.f, colliders, mask);
	is_ok |= skipFront;

	float delta_yaw = player_transform->getYawRotationToAimTo(transform->getPosition());
	is_ok &= fabsf(rad2deg(delta_yaw)) < acceptance_angle;

	if (rotateToFace)
	{
		TCompTransform* player_transform = player->get<TCompTransform>();
		targetRotation = player_transform->getRotation() * QUAT::CreateFromYawPitchRoll(delta_yaw, 0.f, 0.f);
	}

	// is_ok &= player_transform->getForward().Dot(transform->getForward()) < 0.f;

	VEC3 pos = transform->getPosition() + offset;
	VEC3 playerPos = player_transform->getPosition();

	if (useLimits)
	{
		is_ok &= playerPos.x > Xlimits.x && playerPos.x < Xlimits.y;
		is_ok &= playerPos.z > Zlimits.x && playerPos.z < Zlimits.y;
	}
	else
	{
		// float dist = VEC2::Distance(VEC2(pos.x, pos.z), VEC2(playerPos.x, playerPos.z));
		float dist = VEC3::Distance(pos, playerPos);
		is_ok &= dist < acceptance_dist;
	}
	
	return is_ok;
}

void TCompLaunchAnimation::launch()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return;

	enabled = true;

	TCompPlayerController* c_controller = player->get<TCompPlayerController>();

	if (useInteractCam)
		c_controller->blendCamera("camera_interact", 3.0f, &interpolators::cubicInOutInterpolator);
	
	if (useVariableValue)
		c_controller->setVariable(animation_variable, variable_value);
	else // Don't do a binary op it will fail bc of the variant
		c_controller->setVariable(animation_variable, true);
	
	PlayerInteraction.setAnimationLauncher(CHandle(this));

	// Set new masks
	TCompCollider* c_collider = get<TCompCollider>();

	if (new_group.length())
	{
		c_collider->setGroupAndMask(new_group, "all");
	}
	else
	{
		c_collider->setGroupAndMask("interactable", "all_but_player");
	}
	
	TCompTransform* transform = get<TCompTransform>();
	VEC3 new_pos = transform->getPosition() + transform->getForward() * 0.001f;
	transform->setPosition(new_pos);
	c_collider->setGlobalPose(transform->getPosition(), transform->getRotation());
}

void TCompLaunchAnimation::oncomplete()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return;

	PlayerInteraction.setAnimationLauncher(CHandle());
	PlayerInteraction.setActive(false);

	// to avoid Eon exiting the energy wall using time reversal
	TCompTimeReversal* c_time_reversal = player->get<TCompTimeReversal>();
	c_time_reversal->clearBuffer();

	// enable collisions
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setGroupAndMask("interactable", "all");

	if (useInteractCam)
	{
		TCompPlayerController* c_controller = player->get<TCompPlayerController>();
		c_controller->blendCamera("camera_follow", 1.2f, &interpolators::cubicInOutInterpolator);
	}
}