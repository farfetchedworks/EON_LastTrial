#pragma once

#include "modules/module.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "components/common/comp_collider.h"
#include "PxPhysicsAPI.h"

extern CTransform toTransform(physx::PxTransform pxtrans);
extern physx::PxTransform toPxTransform(CTransform mcvtrans);

class CModulePhysics : public IModule
{
public:
	enum FilterGroup {
		None = 0,
		Wall = 1 << 0,
		Floor = 1 << 1,
		Player = 1 << 2,
		Enemy = 1 << 3,
		Projectile = 1 << 4,
		Interactable = 1 << 5,
		Door = 1 << 6,
		EffectArea = 1 << 7,
		Weapon = 1 << 8,
		Trigger = 1 << 9,
		Prop = 1 << 10,
		InvisibleWall = 1 << 11,
		Scenario = Wall | Floor | Door,
		Characters = Player | Enemy,
		All = -1,
		AllButPlayer = All ^ Player | Scenario,
		AllButEffectArea = All ^ EffectArea
	};

	physx::PxScene* gScene = NULL;
	static physx::PxQueryFilterData defaultFilter;

	FilterGroup getFilterByName(const std::string& name);
	physx::PxU32 readFilterFromJson(std::string mask_string);

	void setSimulationDisabled(physx::PxRigidActor* actor, bool disabled);
	void setupFiltering(physx::PxShape* shape, physx::PxU32 filterGroup, physx::PxU32 filterMask);
	void setupFilteringOnAllShapesOfActor(physx::PxRigidActor* actor, physx::PxU32 filterGroup, physx::PxU32 filterMask);
	void getFilteringFromShape(physx::PxShape* shape, physx::PxU32& filterGroup, physx::PxU32& filterMask);
	void getFilteringFromAllShapesofActor(physx::PxRigidActor* actor, physx::PxU32 filterGroup[], physx::PxU32 filterMask[]);
	void createActor(TCompCollider& comp_collider);
	bool readShape(physx::PxRigidActor* actor, const json& jcfg);
	physx::PxRigidActor* createController(TCompCollider& comp_collider, bool isCapsuleController);
	physx::PxRigidActor* createCapsuleController(TCompCollider& comp_collider);
	physx::PxRigidActor* createBoxController(TCompCollider& comp_collider);
	physx::PxMaterial* createMaterial(const VEC3& mat);

	void removeActors(TCompCollider& comp_collider);

	CModulePhysics(const std::string& name) : IModule(name) { }
	bool start() override;
	void stop() override;
	void render() override;
	void update(float delta) override;
	void renderDebug() override;
	void renderInMenu() override;

	struct DBG_Line {
		float remove = false;
		float timer = 0.f;
		float duration = 5.f;
		bool has_hit = false;
		VEC3 origin;
		VEC3 hit;
		VEC3 end;
		Color hit_color = Color(1.f, 0.f, 0.f, 1.f);
		Color end_color = Color(0.f, 1.f, 0.f, 1.f);
	};

	std::vector<DBG_Line> _debugLines;

	// Creates a raycast and saves the results in a colliders vector and gives THE FIRST HIT
	bool raycast(const VEC3& origin, const VEC3& dir, float distance, VHandles& colliders, VEC3& first_hit, physx::PxU32 hitMask = CModulePhysics::FilterGroup::All);

	// Creates a raycast and saves the results in a colliders vector
	bool raycast(const VEC3& origin, const VEC3& dir, float distance, VHandles& colliders, physx::PxU32 hitMask = CModulePhysics::FilterGroup::All, bool getClosestHit = false);

	// Creates a raycast and returns a raycast buffer to get information about each hit
	bool raycast(const VEC3& origin, const VEC3& dir, float distance, std::vector<physx::PxRaycastHit>& raycastHits,
		physx::PxU32 hitMask = CModulePhysics::FilterGroup::All, bool getClosestHit = false, bool render_debug = true);

	bool sweep(physx::PxTransform initial_pose, const VEC3& dir, float distance, const physx::PxGeometry& shape, std::vector<physx::PxSweepHit>& sweepHits,
		physx::PxU32 hitMask = CModulePhysics::FilterGroup::All, bool getClosestHit = false, bool render_debug = true);
	// ...
	bool overlapSphere(VEC3 position, float radius, VHandles& colliders, physx::PxU32 hitMask = CModulePhysics::FilterGroup::All);

	// Retrieve the physical material right under the actor
	const CPhysicalMaterial* getPhysicalMaterialOfFloor(CHandle actor, VEC3& floor_hit);

	class CustomSimulationEventCallback : public physx::PxSimulationEventCallback
	{
		// Implements PxSimulationEventCallback
		virtual void							onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs);
		virtual void							onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count);
		virtual void							onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) {}
		virtual void							onWake(physx::PxActor**, physx::PxU32) {}
		virtual void							onSleep(physx::PxActor**, physx::PxU32) {}
		virtual void							onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, const physx::PxU32) {}
	};

	CustomSimulationEventCallback customSimulationEventCallback;


	/*
	  Query filter callbacks
	*/

	// Query filter callback for Touches
	class CustomQueryFilterCallback : public physx::PxQueryFilterCallback
	{
		// Implements PxQueryFilterCallback
		virtual physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags);
		virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) { return physx::PxQueryHitType::eBLOCK; };
	};
	CustomQueryFilterCallback customQueryFilterCallback;

	// Query filter callback for Blocking hits
	class BlockingQueryFilterCallback : public physx::PxQueryFilterCallback
	{
		// Implements PxQueryFilterCallback
		virtual physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags);
		virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) { return physx::PxQueryHitType::eBLOCK; };
	};
	BlockingQueryFilterCallback blockingQueryFilterCallback;


	/*
	  Character Controllers custom hits and filters
	  * Character controller filter: to detect collisions between controllers and objects
	  * Hit controller: events to be executed when the controller hits shapes, controllers or obstacles
	*/


	// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/CharacterControllers.html#character-interactions-cct-vs-cct
	class CustomCharacterControllerFilter : public physx::PxControllerFilterCallback {

		virtual bool PxControllerFilterCallback::filter(const physx::PxController& a, const physx::PxController& b);
	};
	CustomCharacterControllerFilter collisionFilter;

	// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/CharacterControllers.html#hit-callback
	class CustomHitController : public physx::PxUserControllerHitReport
	{
		// Implements PxUserControllerHitReport
		virtual void							onShapeHit(const physx::PxControllerShapeHit& hit);
		virtual void							onControllerHit(const  physx::PxControllersHit& hit);
		virtual void							onObstacleHit(const  physx::PxControllerObstacleHit& hit);
	};
	CustomHitController customHitController;
};
