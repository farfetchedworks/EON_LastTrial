#include "mcv_platform.h"
#include "engine.h"
#include "comp_weapon_part_trigger.h"
#include "render/render_module.h"
#include "modules/module_physics.h"
#include "render/draw_primitives.h"
#include "lua/module_scripting.h"
#include "fsm/states/logic/state_logic.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_hierarchy.h"
#include "components/common/comp_fsm.h"
#include "components/common/comp_updates_physics.h"
#include "components/gameplay/comp_game_manager.h"
#include "skeleton/comp_attached_to_bone.h"
#include "entity/entity_parser.h"
#include "audio/module_audio.h"
#include "../bin/data/shaders/constants_particles.h"

DECL_OBJ_MANAGER("weapon_part_trigger", TCompWeaponPartTrigger)

void TCompWeaponPartTrigger::load(const json& j, TEntityParseContext& ctx)
{
	is_point = j.value("is_point", false);
	offset = loadVEC3(j, "offset");
}

void TCompWeaponPartTrigger::onEntityCreated()
{
	h_hierarchy = get<TCompHierarchy>();
	h_attached_to_bone = get<TCompAttachedToBone>();
    TCompHierarchy* c_hierarchy = get<TCompHierarchy>();
    CEntity* eParent = c_hierarchy->h_parent_transform.getOwner();
    eParent->sendMsg(TMsgRegisterWeapon({ CHandle(this).getOwner(), false }));

	parent = eParent;
}

void TCompWeaponPartTrigger::update(float dt) {

#ifdef _DEBUG
	// Don't do anything in debug
	return;
#endif

	if (!enabled || !is_point)
		return;

	// Discard trail on slow debuff
	{
		if (Time.scale_factor != 1.f)
			return;
	}

	CEntity* eParent = parent;
	TCompFSM* fsm = eParent->get<TCompFSM>();
	assert(fsm);
	fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
	assert(currState);
	fsm::CContext& ctx = fsm->getCtx();
	
	if (!currState->inActiveFrames(ctx))
		return;

	CTransform newT;
	TCompAttachedToBone* c_attached_to_bone = h_attached_to_bone;
	c_attached_to_bone->applyOffset(newT, offset);
	newT.setRotation(QUAT::Identity);

	VEC3 newPos = newT.getPosition();
	size_t numKnots = knots.size();

	if (numKnots > 0 && VEC3::Distance(knots[numKnots - 1], newPos) < 0.3f)
		return;

	// Weapon trail 
	if (numKnots < 4)
	{
		knots.push_back(newPos);
	}
	else
	{
		assert(numKnots == 4);
		float segments = 35.f;
		float max_frames = segments * 35.f;

		for (float i = 0.f; i <= 1.f; i += (1.f / segments)) {

			const VEC3 pos = VEC3::CatmullRom(knots[0], knots[1],
				knots[2], knots[3], i);

			CTransform fT;
			fT.setPosition(pos);
			CEntity* e = spawn("data/particles/compute_sword_trail_particles.json", fT);
			TCompBuffers* buffers = e->get<TCompBuffers>();
			CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));
			
			float factor = clampf(frames / max_frames, 0.f, 1.f);
			cte->emitter_initial_scale_avg *= std::min(factor, 0.3f);

			// discard it
			if (cte->emitter_initial_scale_avg < 0.05)
				e->destroy();
			
			for (auto& c : cte->psystem_colors_over_time) {
				c.w *= (1 - factor);
			}

			cte->updateFromCPU();
			frames++;
		}

		// Add new position
		knots.push_back(newPos);

		// Remove first point so we have always 4
		knots.pop_front();
	}
}

void TCompWeaponPartTrigger::onSetActive(const TMsgEnableWeapon& msg)
{
	enabled = msg.enabled;
	current_action = msg.action;
	static_cast<TCompCollider*>(get<TCompCollider>())->disable(!msg.enabled);

	// Clear info for trail
	{
		frames = 0;
		knots.clear();
	}
}

VEC3 aux;

void TCompWeaponPartTrigger::spawnFloorParticles()
{
	if (h_floor_parts.isValid())
		return;

	CTransform t;
	TCompAttachedToBone* c_attached_to_bone = h_attached_to_bone;
	c_attached_to_bone->applyOffset(t, offset);

	VEC3 newPos = t.getPosition();

	{
		std::vector<physx::PxRaycastHit> raycastHits;
		if (EnginePhysics.raycast(newPos + VEC3::Up, VEC3::Down, 5.f, raycastHits, CModulePhysics::FilterGroup::Scenario, true)) {
			newPos = PXVEC3_TO_VEC3(raycastHits[0].position);
		}
	}

	aux = newPos;

	t.setPosition(newPos);
	t.setRotation(QUAT::Identity);

	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	bool isCave = gm->getPlayerLocation() == eLOCATION::CAVE;

	CEntity* e = spawn( isCave ? 
		"data/particles/eon_cavefloor_particles.json" : "data/particles/eon_templefloor_particles.json", t);
	TCompBuffers* buffers = e->get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));

	CEntity* eParent = parent;
	cte->emitter_initial_pos = newPos;
	// Use this as direction
	VEC3 dir = newPos - eParent->getPosition();
	dir.Normalize();
	cte->emitter_owner_position = dir;
	cte->updateFromCPU();

	h_floor_parts = e;
}

void TCompWeaponPartTrigger::onTriggerEnter(const TMsgEntityTriggerEnter& msg)
{
	CEntity* eParent = parent;
	TCompTransform* c_player_trans = eParent->get<TCompTransform>();
	TCompCollider* c_collider = eParent->get<TCompCollider>();
	TCompFSM* fsm = eParent->get<TCompFSM>();

	if (!enabled)
		return;

	// Hacky: discard collision with floor
	{
		CEntity* owner = CHandle(this).getOwner();
		CTransform newT;
		TCompAttachedToBone* c_attached_to_bone = h_attached_to_bone;
		c_attached_to_bone->applyOffset(newT, offset);
		VEC3 weaponPos = newT.getPosition();

		// Collision with floor (for sure?)
		if (weaponPos.y < (eParent->getPosition().y + 1.f)) {

			if (!is_point) {
				spawnFloorParticles();
			}

			return;
		}
	}

	if (!is_point) {

		fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
		assert(currState);
		fsm::CContext& ctx = fsm->getCtx();

		if (!currState->inActiveFrames(ctx))
			return;

		float distance = -1.f;
		VEC3 base_ray_pos = c_player_trans->getPosition() + VEC3::Up;
		bool is_stunned = c_collider->collisionAtDistance(base_ray_pos, c_player_trans->getForward(), 10.f, distance) && (distance <= 0.85f);
		fsm->getCtx().setVariableValue("is_stunned", is_stunned);
		static_cast<TCompCollider*>(get<TCompCollider>())->disable(true);
		enabled = false;
	}
	
	CTransform t;
	TCompAttachedToBone* c_attached_to_bone = h_attached_to_bone;
	c_attached_to_bone->applyOffset(t, offset);
	t.setRotation(QUAT::Identity);
	spawn("data/particles/compute_sword_sparks_particle.json", t);

	CEntity* flash = spawn("data/prefabs/flash_light.json", t);
	static unsigned int flashes = 0;
	flash->setName((std::string(flash->getName()) + std::to_string(flashes++)).c_str());
	EngineLua.executeScript("destroyEntity('" + std::string(flash->getName()) + "')", 0.1f);

	// FMOD event
	EngineAudio.postEvent("CHA/Eon/AT/Eon_Hit_Env", CHandle(this).getOwner());
}

void TCompWeaponPartTrigger::debugInMenu()
{
	ImGui::DragFloat3("Offset", &offset.x, 0.01f, -5.f, 5.f);
}

void TCompWeaponPartTrigger::renderDebug()
{
#ifdef _DEBUG
	CTransform newT;
	TCompAttachedToBone* c_attached_to_bone = h_attached_to_bone;
	c_attached_to_bone->applyOffset(newT, offset);
	drawAxis(newT.asMatrix());
#endif

	// drawAxis(MAT44::CreateTranslation(aux));
}