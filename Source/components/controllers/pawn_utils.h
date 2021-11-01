#pragma once

class CEntity;

struct PawnUtils {

	static bool isInsideCone(CHandle h_me, CHandle h_other, float fov, float radius);
	static bool isInsideConeOfBone(CHandle h_me, CHandle h_other, const std::string& bone_name, float fov, float radius);
	static bool hasHeardPlayer(CHandle h_me, float max_hearing_dist, float hear_speed_th);
	
	static void movePhysics(CHandle h_collider, VEC3 move_dir, float dt, float gravity = -9.81f);

	static float distToPlayer(CHandle h_me, bool squared = false);

	static void setPosition(CHandle h_me, VEC3 position);

	static void playAction(CHandle h_me, const std::string& name, float blendIn = 0.25f, float blendOut = 0.25f,
		float weight = 1.f, bool root_motion = false, bool root_yaw = false, bool lock = false);
	static void playCycle(CHandle h_me, const std::string& name, float blend, float weight = 1.f);
	static void stopAction(CHandle h_me, const std::string& name, float blendOut = 0.25f);
	static void stopCycle(CHandle h_me, const std::string& name, float blendOut = 0.25f);

	static void spawnGeons(VEC3 pos, CHandle h_owner);
	static void spawnHealParticles(VEC3 pos, const std::string& name);
};
