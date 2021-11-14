#include "mcv_platform.h"
#include "engine.h"
#include "comp_rigid_animation_controller.h"
#include "components/common/comp_name.h"
#include "components/common/comp_hierarchy.h"
#include "components/common/comp_parent.h"
#include "lua/module_scripting.h"
#include "render/draw_primitives.h"
#include "modules/module_physics.h"
#include "bt/task_utils.h"

// -----------------------------------------------
class CAnimationDataResourceType : public CResourceType
{
public:
	const char* getExtension(int idx) const { return ".anim"; }
	const char* getName() const { return "Animations"; }
	IResource* create(const std::string& name) const
	{
		TFileContext file_context(name);
		TCoreAnimationData* obj = new TCoreAnimationData();
		CFileDataProvider fdp(name.c_str());
		bool is_ok = obj->read(fdp);
		assert(is_ok);
		return obj;
	}
};

template<>
CResourceType* getClassResourceType<TCoreAnimationData>()
{
	static CAnimationDataResourceType factory;
	return &factory;
}

// -----------------------------------------------

DECL_OBJ_MANAGER("animator_controller", TCompRigidAnimationController)

void TCompRigidAnimationController::load(const json& j, TEntityParseContext& ctx)
{
	animation_src = j.value("src", std::string());
	if (!animation_src.empty())
		animation_data = Resources.get(animation_src)->as<TCoreAnimationData>();
	speed_factor = j.value("speed_factor", speed_factor);
	frame_start = j.value("frame_start", frame_start);
	loop = j.value("loop", loop);
	dynamic = j.value("dynamic", dynamic);
	autoplay = j.value("autoplay", autoplay);
	animate_prefabs = j.value("animate_prefabs", animate_prefabs);
	cinematic_animation = j.value("cinematic_animation", cinematic_animation);
	reverse = j.value("reverse", reverse);
}

void TCompRigidAnimationController::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
	if (!animation_src.size())
		return;
	
	setAnimation(animation_src);

	if (autoplay)
		start();
}

void TCompRigidAnimationController::setTarget(const std::string& target_name)
{
	cinematic_target = getEntityByName(target_name);
}

void TCompRigidAnimationController::setAnimation(const std::string& animation_name)
{
	animation_data = Resources.get(animation_name)->as<TCoreAnimationData>();
	assignTracksToSceneObjects();
	
	if (!reverse)
	{
		curr_time = animation_data->header.min_time;
	}
	else
	{
		curr_time = animation_data->header.max_time;
	}

	for (auto t : tracks)
		t.apply(curr_time);
}

float TCompRigidAnimationController::getAnimationTime()
{
	if (!animation_data)
		return 0.f;

	return std::max(animation_data->header.max_time - animation_data->header.min_time, 0.f) / speed_factor;
}

void TCompRigidAnimationController::setTargetBone(const std::string& bone_name)
{
	cinematic_target_bone = bone_name;
}

void TCompRigidAnimationController::setSpeed(float speed)
{
	speed_factor = speed;
}

void TCompRigidAnimationController::start()
{
	if (frame_start > 0)
	{
		float time = std::min(frame_start / 30.f, animation_data->header.max_time);
		time = std::max(time, animation_data->header.min_time);
		curr_time = time;
	}

	playing = true;
}

void TCompRigidAnimationController::stop()
{
	playing = false;
}

void TCompRigidAnimationController::update(float delta_time)
{
	if (!playing)
		return;

	if (dynamic)
	{
		for (auto& t : tracks)
		for (auto& o : t.objs) {
			if (strcmp(t.core_track->property_name, "transform") != 0)
				continue;
			CEntity* player = getEntityByName("player");
			TCompTransform* transform = player->get<TCompTransform>();
			o.initial_pose.fromMatrix(*transform);
		}
	}

	for (auto t : tracks)
		t.apply(curr_time);

	if (!reverse)
	{
		if (curr_time <= animation_data->header.max_time)
		{
			// We can only exit if we know for sure we have applied the max_time to all objects
			if (curr_time == animation_data->header.max_time)
				onEndOfAnimation();
			else
				updateCurrentTime(delta_time);
		}
	}
	else {
		if (curr_time >= animation_data->header.min_time)
		{
			if (curr_time == animation_data->header.min_time)
				onEndOfAnimation();
			else
				updateCurrentTime(delta_time);
		}
	}

	if (cinematic_animation && cinematic_target.isValid()) {

		TCompTransform* c_target = get<TCompTransform>();
		assert(c_target);
		CEntity* e = cinematic_target;
		TCompTransform* target_transform = e->get<TCompTransform>();
		VEC3 target_pos = e->getPosition();

		if (cinematic_target_bone.size()) {
			target_pos = TaskUtils::getBoneWorldPosition(e, cinematic_target_bone);
		}

		VEC3 position = c_target->getPosition();

		// Don't leave the scenario
		{
			VEC3 rayDir = position - target_pos;
			float dist_to_player = rayDir.Length();
			rayDir.Normalize();
			physx::PxU32 layerMask = CModulePhysics::FilterGroup::Scenario;
			std::vector<physx::PxSweepHit> sweepHits;
			physx::PxTransform trans(VEC3_TO_PXVEC3(target_pos));
			float sphere_radius = 0.1f; // Should be equal to camera's z near
			physx::PxSphereGeometry geometry = { sphere_radius };
			bool hasHit = EnginePhysics.sweep(trans, rayDir, dist_to_player, geometry, sweepHits, layerMask, true, false);
			if (hasHit) {
				position = PXVEC3_TO_VEC3(sweepHits[0].position) + PXVEC3_TO_VEC3(sweepHits[0].normal) * sphere_radius;
			}
		}

		c_target->lookAt(position, target_pos, VEC3::Up);
	}
}

void TCompRigidAnimationController::assignTracksToSceneObjects()
{
	// resolve the tracks from the animation data
	tracks.clear();
	for (auto& ct : animation_data->tracks)
	{
		TTrack t;
		
		if (animate_prefabs) {
			auto it = TCompName::all_prefabs.find(ct.obj_name);
			if (it != TCompName::all_prefabs.end()) {

				VHandles hs = it->second;

				for (auto h : hs) {
					if (!h.isValid())
						continue;
					TTrackObject tObj;
					tObj.handle = h;
					t.objs.push_back(tObj);
				}

			}
			else {
				CEntity* e = getEntityByName(ct.obj_name);
				if (!e)
					continue;
				TCompHierarchy* comp_hierarchy = e->get<TCompHierarchy>();
				assert(comp_hierarchy);

				auto it = TCompName::all_prefabs.find(comp_hierarchy->parent_name);
				assert(it != TCompName::all_prefabs.end());

				VHandles hs = it->second;

				for (auto h : hs) {
					CEntity* prefab_parent = h;
					if (!prefab_parent)
						continue;
					TCompParent* comp_parent = prefab_parent->get<TCompParent>();
					assert(comp_parent);
					CHandle child = comp_parent->getChildByName(ct.obj_name);
					assert(child.isValid());

					TTrackObject tObj;
					tObj.handle = child;
					t.objs.push_back(tObj);
				}
			}
		}
		else {

			TTrackObject tObj;

			if (cinematic_animation) {
				tObj.handle = getEntity();
			}
			else {
				tObj.handle = getEntityByName(ct.obj_name);
			}

			if (!tObj.handle.isValid())
				continue;
			t.objs.push_back(tObj);
		}

		for (auto& o : t.objs) {
			CEntity* e_obj = o.handle;
			if (strcmp(ct.property_name, "transform") == 0)
			{
				TCompTransform* transform = e_obj->get<TCompTransform>();
				o.handle = transform;

				if (cinematic_animation) {
					assert(cinematic_target.isValid());
					CEntity* target = cinematic_target;
					TCompTransform* t_transform = target->get<TCompTransform>();
					CEntity* player = getEntityByName("player");
					CTransform t;
					t.setPosition(t_transform->getPosition());
					float yaw = t_transform->getYawRotationToAimTo(player->getPosition());
					t.setRotation(QUAT::Concatenate(QUAT::CreateFromYawPitchRoll(yaw, 0.f, 0.f), t_transform->getRotation()));
				
					o.initial_pose = t;
				}
				else {
					o.initial_pose.fromMatrix(*transform);
				}
			}
			else if (strcmp(ct.property_name, "notes") == 0)
			{
				// Nothing special, notes sent to the entity
			}
			else
			{
				// Implement other track types.. Camera info?, blur info? events?
				fatal("Don't know to how to update property name %s\n", ct.property_name);
				continue;
			}
		}

		t.core_track = &ct;
		tracks.push_back(t);
	}
}

void TCompRigidAnimationController::updateCurrentTime(float delta_time)
{
	curr_time += delta_time * speed_factor * (reverse ? -1.0f : 1.0f);
	// Never go beyond the time
	if (curr_time > animation_data->header.max_time)
		curr_time = animation_data->header.max_time;
	else if (curr_time < animation_data->header.min_time)
		curr_time = animation_data->header.min_time;
}

void TCompRigidAnimationController::onEndOfAnimation()
{
	// loop? / change_direction? / disable
	if (loop)
	{
		if (ping_pong)
			reverse = !reverse;

		if (!reverse)
			curr_time = animation_data->header.min_time;
		else
			curr_time = animation_data->header.max_time;

		for (auto& t : tracks)
			t.rewind();
	}
	else {
		playing = false;

		if (cinematic_animation) {
			EngineLua.executeScript("stopCinematic(1.0)");
		}
	}
}

void TCompRigidAnimationController::debugInMenu()
{
	if (ImGui::DragFloat("Current Time", &curr_time, 0.01f, animation_data->header.min_time, animation_data->header.max_time))
	{
		for (auto t : tracks)
			t.apply(curr_time);
	}
		
	ImGui::Separator();
	if (ImGui::Checkbox("Playing", &playing))
	{
		if (playing)
			start();
	}
	ImGui::Checkbox("Reverse", &reverse);
	ImGui::Checkbox("Loop", &loop);
	if (loop)
		ImGui::Checkbox("Ping Pong", &ping_pong);
	ImGui::DragFloat("Speed Factor", &speed_factor, 0.01f, -5.0f, 5.f);
}