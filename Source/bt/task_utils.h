#pragma once
#include "fsm/fsm.h"

class TCompTransform;
class TCompAIControllerBase;
class CHandle;
class CEntity;
class CBTContext;

struct TaskUtils {
	static float rotation_th;

	static void moveForward(CBTContext& ctx, const float speed, const float dt);
	static VEC3 moveAnyDir(CBTContext& ctx, const VEC3 dst, const float speed, const float dt, float max_dist = 0.f);
	static void rotateToFace(TCompTransform* my_trans, const VEC3 target, const float speed, const float dt);
	static void changeLookAtAngle(CEntity* e_observer, const float angle);
	static float rotate(TCompTransform* my_trans, const float speed, const float dt);
	static void hit(CEntity* striker, CEntity* target, int damage);
	static bool canSeePlayer(TCompTransform* my_trans, TCompTransform* player_trans);
	static bool hasObstaclesToEon(TCompTransform* my_trans, TCompTransform* player_trans);

	static void castAreaAttack(CEntity* e_attacker, VEC3 position, const float radius, const int dmg);
	static void castAreaDelay(CEntity* e_caster, float duration, CHandle h_locked_t);
	static void castWaveDelay(CEntity* e_caster, float initial_radius, float max_radius, float duration);

	static VEC3 getRandomPosAroundPoint(const VEC3& position, float range);
	static VEC3 getBoneWorldPosition(CHandle entity, const std::string& bone_name);
	static VEC3 getBoneForward(CHandle entity, const std::string& bone_name);

	static void setSkelLookAtEnabled(CEntity* e, bool enabled);

	static void playAction(CEntity* owner, const std::string& name, float blendIn = 0.25f, float blendOut = 0.25f,
		float weight = 1.f, bool root_motion = false, bool root_yaw = false, bool lock = false);
	static void playCycle(CEntity* owner, const std::string& name, float blend = 0.25f, float weight = 1.f);

	static void stopAction(CEntity* owner, const std::string& name, float blendOut = 0.25f);
	static void stopCycle(CEntity* owner, const std::string& name, float blendOut = 0.25f);

	static void pauseAction(CBTContext& ctx, const std::string& name, std::function<void(CBTContext& ctx)> cb = nullptr);
	static void resumeAction(CBTContext& ctx, const std::string& name);
	static void resumeActionAt(CBTContext& ctx, const std::string& name, float time, float dt);

	static void dissolveAt(CBTContext& ctx, float time, float wait_time = 1.f, bool propagate_childs = true);

	static void spawnBranch(VEC3 position, const int dmg, const float speed);

	static void spawnProjectile(const VEC3 shoot_orig, const VEC3 shoot_target, const int dmg, const bool from_player, bool is_homing = false);
	static void spawnCygnusProjectiles(const VEC3 shoot_orig, const VEC3 shoot_target, const int dmg, const int num_projectiles);
	static void spawnParticles(const std::string& name, VEC3 position, float radius, int iterations = 1, int num_particles = -1);
	static void spawnCygnusForm1Clone(VEC3 position, float lifespan = 10.f);

	static CHandle getPlayer()
	{
		CEntity* playerEntity = getEntityByName("EonHolo");

		if (!playerEntity) {
			playerEntity = getEntityByName("player");
		}

		return playerEntity;
	}

private:
	
	static fsm::CContext& getFSMCtx(CBTContext& ctx);
};
