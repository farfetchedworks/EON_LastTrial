#pragma once
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "animation/rigid/rigid_animation_data.h"
#include "components/messages.h"

struct TCompRigidAnimationController : public TCompBase
{
	DECL_SIBLING_ACCESS();

	// What is being moved/animated???
	struct TTrackObject {
		CHandle handle;
		CTransform initial_pose;
	};

	struct TTrack {
		
		// Detailed information to gather from the animation data to update handle
		const TCoreAnimationData::TCoreTrack* core_track;
		uint32_t next_key = 0;
		std::vector<TTrackObject> objs;

		void rewind()
		{
			next_key = 0;
			apply(0.0f);
		}

		void apply(float current_time)
		{
			if (!objs.size())
				return;

			if (core_track->keys_type == TCoreAnimationData::TCoreTrack::eKeyType::TRANSFORM)
			{
				float ut = 0.0;

				if (current_time < core_track->min_time)
					ut = 0.0f;
				else if (current_time > core_track->max_time)
					ut = 1.0f;
				else
					ut = (current_time - core_track->min_time) / (core_track->max_time - core_track->min_time);

				// find the two nearest keys...
				float t = core_track->num_keys * ut;
				int prev_frame = (int)t;		// remove decimals
				int next_frame = prev_frame + 1;
				prev_frame = std::max(0, std::min(prev_frame, (int)core_track->num_keys - 1));
				next_frame = std::max(0, std::min(prev_frame + 1, (int)core_track->num_keys - 1));

				const auto& key0 = core_track->getKey<CTransform>(prev_frame);
				const auto& key1 = core_track->getKey<CTransform>(next_frame);
				float amount_of_1 = t - prev_frame;

				for (auto o : objs) {
					TCompTransform* c_trans = o.handle;
					assert(c_trans);
					CTransform tr = key0;
					tr.interpolateTo(key1, amount_of_1);
					c_trans->fromMatrix(o.initial_pose.combinedWith(tr));
				}
			}
			else if (core_track->keys_type == TCoreAnimationData::TCoreTrack::eKeyType::TIMED_NOTE)
			{
				// Only works when playing forward...
				while (next_key < core_track->num_keys)
				{
					const auto& note = core_track->getKey<TCoreAnimationData::TTimedNote>(next_key);
					if (current_time < note.time)
						break;
					// dbg("  Animation event %d at time:%f Curr:%f Text: %s\n", next_key, note.time, current_time, note.getText(core_track->var_data));
					++next_key;
				}

			}
		}
	};

	const TCoreAnimationData* animation_data = nullptr;
	std::string           animation_src;
	std::vector< TTrack > tracks;
	float                 curr_time = 0.0f;
	float                 speed_factor = 1.0f;
	bool                  playing = false;
	bool                  loop = false;
	bool                  ping_pong = false;
	bool                  reverse = false;
	bool                  autoplay = false;
	bool                  animate_prefabs = false;
	bool                  dynamic = false;
	int					  frame_start = 0;

	CHandle				  cinematic_target;
	bool                  cinematic_animation = false;
	std::string			  cinematic_target_bone;

	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();
	void renderDebug();
	void start();
	void stop();
	void update(float delta_time);
	void assignTracksToSceneObjects();
	void updateCurrentTime(float delta_time);
	void onEndOfAnimation();

	void setTarget(const std::string& target_name);
	void setAnimation(const std::string& animation_name);
	void setTargetBone(const std::string& bone_name);
	void setSpeed(float speed);

	bool hasTracks() { return tracks.size() > 0; }
	float getAnimationTime();

	static void registerMsgs() {
		DECL_MSG(TCompRigidAnimationController, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}

	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
};