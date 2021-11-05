#include "mcv_platform.h"
#include "comp_collider.h"
#include "modules/module_physics.h"
#include "engine.h"
#include "render/render.h"
#include "render/draw_primitives.h"
#include "components/common/comp_transform.h"
#include "engine.h"


DECL_OBJ_MANAGER("collider", TCompCollider)

//void CObjectManager<TCompCollider>u::pdateAll(float dt) {
//	// do something special...
//}

using namespace physx;

TCompCollider::~TCompCollider()
{
	if (actor && EnginePhysics.isActive())
	{
		releaseController();
		if (actor) actor->release(), actor = nullptr;
	}

	if (force_actor && EnginePhysics.isActive())
	{
		force_actor->release();
		force_actor = nullptr;
	}
}

void TCompCollider::releaseController()
{
	if (cap_controller)
	{
		cap_controller->release();
		cap_controller = nullptr;
		// No need to release the actor.
		actor = nullptr;
	}

	if (box_controller) {
		box_controller->release();
		box_controller = nullptr;
		// No need to release the actor.
		actor = nullptr;
	}
}

void TCompCollider::debugInMenu()
{
	auto actor_type = actor->getType();
	if (actor_type == physx::PxActorTypeFlag::eRIGID_DYNAMIC)
	{
		ImGui::Text("Rigid Dynamic");
		PxRigidDynamic* rd = (PxRigidDynamic*)actor;
		if (rd->isSleeping())
		{
			ImGui::SameLine();
			ImGui::Text("Sleeping");
		}
	}
	else if (actor_type == physx::PxActorTypeFlag::eRIGID_STATIC)
		ImGui::Text("Rigid Static");

	if (cap_controller)
	{
		float rad = cap_controller->getRadius();
		if (ImGui::DragFloat("Controller Radius", &rad, 0.02f, 0.1f, 5.0f))
			cap_controller->setRadius(rad);
		float height = cap_controller->getHeight();
		if (ImGui::DragFloat("Controller Height", &height, 0.02f, 0.1f, 5.0f))
			cap_controller->setHeight(height);
	}

	// Show current filters from the first shape of the actor
	PxU32 filterGroup[1];
	PxU32 filterMask[1];
	EnginePhysics.getFilteringFromAllShapesofActor(actor, filterGroup, filterMask);
	ImGui::LabelText("Filter Group", "%i", filterGroup[0]);
	ImGui::LabelText("Filter Mask", "%i", filterMask[0]);

	onEachShape(&TCompCollider::debugInMenuShape);
}

void TCompCollider::load(const json& j, TEntityParseContext& ctx) {
	
	assert(j.is_object());
	jconfig = j;

	is_controller = j.value("controller", is_controller);
	is_dynamic = j.value("dynamic", is_dynamic);
	shape = j.value("shape", "sphere");
	radius = j.value("radius", radius);
	height = j.value("height", height);
	mask = j.value("mask", "all");
	simulation_disabled = j.value("simulation_disabled", false);
}

void TCompCollider::onEntityCreated()
{
	EnginePhysics.createActor(*this);
	disable(simulation_disabled);

	if (is_controller)
		createForceActor();

	h_transform = get<TCompTransform>();
}

void TCompCollider::createForceActor()
{
	jconfig.erase("controller");
	jconfig["dynamic"] = true;
	jconfig["shape"] = "capsule";

	EnginePhysics.createActor(*this);
	
	{
		CTransform trans = toTransform(((physx::PxRigidDynamic*)actor)->getGlobalPose());
		physx::PxTransform px_trans(VEC3_TO_PXVEC3(VEC3(0.f, 1000.f, 0.f)), QUAT_TO_PXQUAT(trans.getRotation()));
		float contact_offset = is_capsule_controller ? cap_controller->getContactOffset() : box_controller->getContactOffset();
		PxVec3 height = PxVec3(0, this->height * 0.5f + radius + contact_offset, 0);
		px_trans.p += height;
		force_actor->setGlobalPose(px_trans);
		if (box_controller) box_controller->setFootPosition(PxExtendedVec3(0.f, 1000.0f, 0.f));
		EnginePhysics.setSimulationDisabled(force_actor, true);
		EnginePhysics.setupFilteringOnAllShapesOfActor(force_actor, EnginePhysics.getFilterByName("none"), EnginePhysics.getFilterByName("none"));
	}

	{
		physx::PxRigidDynamic* rd = ((physx::PxRigidDynamic*)force_actor);
		rd->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);
		rd->setMaxDepenetrationVelocity(physx::PxReal(3));
		rd->setMaxContactImpulse(physx::PxReal(3));
		rd->setMass(1.2f);
	}
}

void TCompCollider::setTriggerAs(bool enabled)
{
	EnginePhysics.setSimulationDisabled(actor, true);
	PxU32 nshapes = actor->getNbShapes();

	static const PxU32 max_shapes = 8;
	PxShape* shapes[max_shapes];
	assert(nshapes <= max_shapes);

	// Even when the buffer is small, it writes all the shape pointers
	PxU32 shapes_read = actor->getShapes(shapes, sizeof(shapes), 0);

	// An actor can have several shapes
	for (PxU32 i = 0; i < nshapes; ++i)
	{
		PxShape* shape = shapes[i];
		assert(shape);
		shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, enabled);
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !enabled);
	}

	EnginePhysics.setupFilteringOnAllShapesOfActor(actor,
		EnginePhysics.getFilterByName(jconfig.value("group", "characters")),
		EnginePhysics.getFilterByName(jconfig.value("mask", "all"))
	);

	actor->userData = CHandle(this).asVoidPtr();
	EnginePhysics.setSimulationDisabled(actor, false);
}

void TCompCollider::update(float dt)
{
	if (!is_controller || !active_force)
		return;

	float curr_length = getLinearVelocity().Length();
	bool stopped = (curr_length < min_vel);
	
	//if (stopped && curr_length != .0f && !active_force)
	//	stopFollowForceActor();

	if (!disable_gravity &&  distanceToGround() < 0.3f)
		stopFollowForceActor("dash");

	if (active_force/* && !stopped*/)
		followForceActor();
}

// --------------------------------------------------------------------
void TCompCollider::debugInMenuShape(physx::PxShape* shape, physx::PxGeometryType::Enum geometry_type, const void* geom, MAT44 world)
{
	PxShapeFlags flags = shape->getFlags();
	if (flags & PxShapeFlag::eTRIGGER_SHAPE)
		ImGui::Text("Is trigger");

	PxTransform t = shape->getLocalPose();
	VEC3 offset = PXVEC3_TO_VEC3(t.p);
	if (ImGui::DragFloat3("Offset", &offset.x, 0.1f, 0.f, 50.f))
	{
		t.p = VEC3_TO_PXVEC3(offset);
		shape->setLocalPose(t);
	}

	switch (geometry_type)
	{
		case PxGeometryType::eSPHERE: {
			PxSphereGeometry* sphere = (PxSphereGeometry*)geom;
			ImGui::LabelText("Sphere Radius", "%f", sphere->radius);
			break;
		}
		case PxGeometryType::eBOX: {
			PxBoxGeometry* box = (PxBoxGeometry*)geom;
			VEC3 dims = PXVEC3_TO_VEC3(box->halfExtents);
			if (ImGui::DragFloat3("Half", &dims.x, 0.1f, 0.f, 50.f))
			{
				setBoxShapeDimensions(dims);
			}
			ImGui::LabelText("Box", "Half:%f %f %f", box->halfExtents.x, box->halfExtents.y, box->halfExtents.z);
			break;
		}
		case PxGeometryType::ePLANE: {
			PxPlaneGeometry* plane = (PxPlaneGeometry*)geom;
			ImGui::Text("Plane");
			break;
		}
		case PxGeometryType::eCAPSULE: {
			PxCapsuleGeometry* capsule = (PxCapsuleGeometry*)geom;
			ImGui::LabelText("Capsule", "Rad:%f Height:%f", capsule->radius, capsule->halfHeight);
			break;
		}
		case PxGeometryType::eTRIANGLEMESH: {
			PxTriangleMeshGeometry* trimesh = (PxTriangleMeshGeometry*)geom;
			ImGui::LabelText("Tri mesh", "%d verts", trimesh->triangleMesh->getNbVertices());
			break;
		}
		case PxGeometryType::eCONVEXMESH: {
			PxConvexMesh* convex_mesh = (PxConvexMesh*)geom;
			ImGui::LabelText("Convex mesh", "%d verts", convex_mesh->getNbVertices());
			break;
		}
	}
}


// ---------------------------------------------------------------------------
void TCompCollider::renderDebug()
{
#ifdef _DEBUG
	onEachShape(&TCompCollider::renderShape);
#endif
}

void TCompCollider::renderShape(physx::PxShape* shape, physx::PxGeometryType::Enum geometry_type, const void* geom, MAT44 world)
{
	VEC4 color = VEC4(1, 0, 0, 1);

	PxShapeFlags flags = shape->getFlags();
	if (flags & PxShapeFlag::eTRIGGER_SHAPE) {

		color = VEC4(1, 1, 0, 1);
		if (!simulation_disabled)
			color = VEC4(0, 1, 0, 1);
	}

	switch (shape->getGeometryType())
	{
		case PxGeometryType::eSPHERE: {
			PxSphereGeometry* sphere = (PxSphereGeometry*)geom;
			drawWiredSphere(world, sphere->radius, color);
			break;
		}

		case PxGeometryType::eBOX: {
			PxBoxGeometry* box = (PxBoxGeometry*)geom;
			AABB aabb;
			aabb.Center = VEC3(0, 0, 0);
			aabb.Extents = PXVEC3_TO_VEC3(box->halfExtents);
			drawWiredAABB(aabb, world, color);
			break;
		}

		case PxGeometryType::ePLANE: {
			PxBoxGeometry* box = (PxBoxGeometry*)geom;
			const CMesh* grid = Resources.get("grid.mesh")->as<CMesh>();
			// To generate a PxPlane from a PxTransform, transform PxPlane(1, 0, 0, 0).
			// Our plane is 0,1,0
			MAT44 Z2Y = MAT44::CreateRotationZ((float)-M_PI_2);
			drawPrimitive(grid, Z2Y * world, VEC4(1, 1, 1, 1));
			break;
		}

		case PxGeometryType::eCAPSULE: {
			PxCapsuleGeometry* capsule = (PxCapsuleGeometry*)geom;
			if (cap_controller)
			{
				MAT44 world1 = MAT44::CreateTranslation(0, 0.f, 0.f) * world;
				drawWiredSphere(world1, capsule->radius, color);
				MAT44 world2 = MAT44::CreateTranslation(-2.0f * capsule->halfHeight, 0.f, 0.f) * world;
				drawWiredSphere(world2, capsule->radius, color);
			}
			else
			{
				// world indicates the center of the cylinder position
				// physics extends the cylinder along the x axis
				// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/files/classPxCapsuleGeometry.html#_details
				MAT44 world1 = MAT44::CreateTranslation(-capsule->halfHeight, 0.f, 0.f) * world;
				drawWiredSphere(world1, capsule->radius, color);
				MAT44 world2 = MAT44::CreateTranslation(capsule->halfHeight, 0.f, 0.f) * world;
				drawWiredSphere(world2, capsule->radius, color);
			}
			// Draw some lines....
			break;
		}

		case PxGeometryType::eCONVEXMESH: {
			const CMesh* render_mesh = (const CMesh*)shape->userData;
			assert(render_mesh);
			drawPrimitive(render_mesh, world, Colors::White);
			break;
		}

		case PxGeometryType::eTRIANGLEMESH: {
			const CMesh* render_mesh = (const CMesh*)shape->userData;
			assert(render_mesh);
			drawPrimitive(render_mesh, world, Colors::White, RSConfig::WIREFRAME);
			break;
		}
	}
}

void TCompCollider::disable(bool disabled)
{
	simulation_disabled = disabled;
	EnginePhysics.setSimulationDisabled(actor, simulation_disabled);
}

void TCompCollider::setGroupAndMask(const std::string& group, const std::string& mask)
{
	EnginePhysics.setupFilteringOnAllShapesOfActor(actor,
		    EnginePhysics.getFilterByName(group),
		    EnginePhysics.getFilterByName(mask)
		);
}

// --------------------------------------------------------------------
// Iterates over all shapes of the actor, calling the given callback
// resolves the invalid cases, and gives the world matrix including the
// shape offset.
void TCompCollider::onEachShape(TShapeFn fn)
{
	// Use the phyics absolute transform to display the physics shapes in debug.
	assert(actor);
	MAT44 actor_transform = toTransform(actor->getGlobalPose()).asMatrix();

	PxU32 nshapes = actor->getNbShapes();

	static const PxU32 max_shapes = 8;
	PxShape* shapes[max_shapes];
	assert(nshapes <= max_shapes);

	// Even when the buffer is small, it writes all the shape pointers
	PxU32 shapes_read = actor->getShapes(shapes, sizeof(shapes), 0);

	// An actor can have several shapes
	for (PxU32 i = 0; i < nshapes; ++i)
	{
		PxShape* shape = shapes[i];
		assert(shape);

		// Combine physics local offset with world transform of the entity
		MAT44 world = toTransform(shape->getLocalPose()).asMatrix() * actor_transform;

		switch (shape->getGeometryType())
		{
			case PxGeometryType::eSPHERE: {
				PxSphereGeometry sphere;
				if (shape->getSphereGeometry(sphere))
					(this->*fn)(shape, PxGeometryType::eSPHERE, &sphere, world);
				break;
			}

			case PxGeometryType::eBOX: {
				PxBoxGeometry box;
				if (shape->getBoxGeometry(box))
					(this->*fn)(shape, PxGeometryType::eBOX, &box, world);
				break;
			}

			case PxGeometryType::ePLANE: {
				PxPlaneGeometry plane;
				if (shape->getPlaneGeometry(plane))
					(this->*fn)(shape, PxGeometryType::ePLANE, &plane, world);
				break;
			}

			case PxGeometryType::eCAPSULE: {
				PxCapsuleGeometry capsule;
				if (shape->getCapsuleGeometry(capsule))
					(this->*fn)(shape, PxGeometryType::eCAPSULE, &capsule, world);
				break;
			}
			case PxGeometryType::eCONVEXMESH: {
				PxConvexMeshGeometry convexmesh;
				if (shape->getConvexMeshGeometry(convexmesh))
					(this->*fn)(shape, PxGeometryType::eCONVEXMESH, &convexmesh, world);
				break;
			}

			case PxGeometryType::eTRIANGLEMESH: {
				PxTriangleMeshGeometry trimesh;
				if (shape->getTriangleMeshGeometry(trimesh))
					(this->*fn)(shape, PxGeometryType::eTRIANGLEMESH, &trimesh, world);
				break;
			}
		}
	}
}

//minVel: minimum velocity to stop force actor (rigidbody)
void TCompCollider::addForce(VEC3 force, const std::string force_origin, bool disableGravity, float minVel)
{
	TCompTransform* transform = h_transform;

	if (is_controller) {
		// activate force actor
		activateForceActor(disableGravity);
		rigidbody = ((physx::PxRigidBody*)force_actor);
	}
	else {
		rigidbody = ((physx::PxRigidBody*)actor);
	}

	if (rigidbody != nullptr) {
		// apply force to rigidbody
		rigidbody->addForce(VEC3_TO_PXVEC3(force), physx::PxForceMode::eIMPULSE);
		applied_forces[force_origin] = force;					// Add the force to the map. Only one force of each type can be handled currently
		forces_num++;					// Increase the amount of applied forces
	}

	min_vel = minVel;
}

void TCompCollider::activateForceActor(bool disable)
{
	//enableCapsuleController();
	//stopFollowForceActor();
	disable_gravity = disable;
	physx::PxTransform PxTrans = ((physx::PxRigidDynamic*)actor)->getGlobalPose();
	((physx::PxRigidDynamic*)force_actor)->setGlobalPose(PxTrans, false);

	EnginePhysics.setupFilteringOnAllShapesOfActor(actor, CModulePhysics::FilterGroup::Player, CModulePhysics::FilterGroup::Scenario);

	EnginePhysics.setupFilteringOnAllShapesOfActor(force_actor, CModulePhysics::FilterGroup::Player, CModulePhysics::FilterGroup::All);
	EnginePhysics.setSimulationDisabled(force_actor, false);
	((physx::PxRigidDynamic*)force_actor)->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, disable_gravity);

	active_force = true;
}

void TCompCollider::followForceActor()
{
	TCompTransform* c_trans = h_transform;
	CTransform trans = toTransform(((physx::PxRigidDynamic*)force_actor)->getGlobalPose());
	
	float distance_to_ground = disable_gravity ? distanceToGround() : .0f;
	distance_to_ground = (distance_to_ground < 3.f) ? distance_to_ground : 0.0f;
	float contactOffset = is_capsule_controller ? cap_controller->getContactOffset() : box_controller->getContactOffset();
	float contact_offset = disable_gravity ? contactOffset : .0f;
	
	// set player position on the ground
	c_trans->setPosition(trans.getPosition() - VEC3(0, height * 0.5f + radius + contact_offset + distance_to_ground, 0));
	setGlobalPose(trans.getPosition() - VEC3(0, distance_to_ground, 0), trans.getRotation());
	is_capsule_controller ? cap_controller->setFootPosition(VEC3_TO_PXEXVEC3(c_trans->getPosition())) :
		box_controller->setFootPosition(VEC3_TO_PXEXVEC3(c_trans->getPosition()));
}

void TCompCollider::stopFollowForceActor(const std::string force_origin)
{
	// Disable simulation to clear all forces
	EnginePhysics.setSimulationDisabled(force_actor, true);

	// If there is more than one force applied, enable the simulation and reapply the forces that should not be removed
	if (forces_num > 1) {
		EnginePhysics.setSimulationDisabled(force_actor, false);

		forces_num--;
		for (auto force: applied_forces){
			if (force.first == force_origin) continue;
			rigidbody->addForce(VEC3_TO_PXVEC3(force.second), physx::PxForceMode::eIMPULSE);
		}
		return;		
	}

	TCompTransform* c_trans = h_transform;

	EnginePhysics.setupFilteringOnAllShapesOfActor(force_actor, CModulePhysics::FilterGroup::None, CModulePhysics::FilterGroup::None);
	EnginePhysics.setupFilteringOnAllShapesOfActor(actor, CModulePhysics::FilterGroup::Player, CModulePhysics::FilterGroup::All);
	((physx::PxRigidDynamic*)force_actor)->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);

	// If this was the last applied force, deactivate all forces and initialize the number of applied forces
	active_force = false;
	applied_forces.clear();
	forces_num = 0;
}

void TCompCollider::setGlobalPose(VEC3 new_pos, QUAT new_rotation, bool autowake)
{
	physx::PxTransform transform(VEC3_TO_PXVEC3(new_pos), QUAT_TO_PXQUAT(new_rotation));
	!active_force ? actor->setGlobalPose(transform, autowake) 
				  : force_actor->setGlobalPose(transform, autowake);
}

void TCompCollider::setBoxShapeDimensions(VEC3 dims)
{
	// Set the radius of the collider as the radius defined in the json
	physx::PxU32 num_shapes = actor->getNbShapes();

	std::allocator<physx::PxShape*> alloc; // allocator for PxShape*
	physx::PxShape** shapes = alloc.allocate(sizeof(physx::PxShape*) * num_shapes);

	// change the radius of all the shapes of the actor 
	// (it only has one, the sphere, but there is no single "getShape")
	actor->getShapes(shapes, num_shapes);
	for (physx::PxU32 i = 0; i < num_shapes; i++)
	{
		physx::PxShape* shape = shapes[i];
		physx::PxBoxGeometry box;
		if (shape->getBoxGeometry(box)) {
			box.halfExtents = VEC3_TO_PXVEC3(dims);
			shape->setGeometry(box);
		}
	}
}

void TCompCollider::setSphereShapeRadius(float radius)
{
	// Set the radius of the collider as the radius defined in the json
	physx::PxU32 num_shapes = actor->getNbShapes();

	std::allocator<physx::PxShape*> alloc; // allocator for PxShape*
	physx::PxShape** shapes = alloc.allocate(sizeof(physx::PxShape*) * num_shapes);	

	// change the radius of all the shapes of the actor 
	// (it only has one, the sphere, but there is no single "getShape")
	actor->getShapes(shapes, num_shapes);
	for (physx::PxU32 i = 0; i < num_shapes; i++)
	{
		physx::PxShape* shape = shapes[i];
		physx::PxSphereGeometry sphere;
		if (shape->getSphereGeometry(sphere)) {
			sphere.radius = radius;
			shape->setGeometry(sphere);
		}
	}
}

bool TCompCollider::collisionAtDistance(const VEC3& org, const VEC3& dir, float maxDistance, float& distance) 
{
	/*CEntity* owner = CHandle(this).getOwner();
	org = owner->getPosition();*/
	physx::PxU32 layerMask = CModulePhysics::FilterGroup::Scenario;
	std::vector<physx::PxRaycastHit> raycastHits;

	// Collision
	bool hasHit = EnginePhysics.raycast(org, dir, maxDistance, raycastHits, layerMask, true, false);
	if (hasHit) {
		distance = raycastHits[0].distance;

		// Collision before maxDistance
		return true;
	}

	// No collision in at least 100m
	return false;
}

float TCompCollider::distanceToGround()
{
	float distance = 0.0f;

	TCompTransform* c_trans = h_transform;

	VEC3 base_ray_pos = c_trans->getPosition() + VEC3::Up;
	physx::PxU32 layerMask = CModulePhysics::FilterGroup::Scenario;
	std::vector<physx::PxRaycastHit> raycastHits;
	
	bool hasHit = EnginePhysics.raycast(base_ray_pos, -VEC3::Up, 80.0f, raycastHits, layerMask, true, true);
	if (hasHit) {
		distance = raycastHits[0].distance;
	}

	return distance > 1.0f ? distance - 1.0f : .0f;
}

VEC3 TCompCollider::getLinearVelocity()
{	
	if (is_controller && active_force)
		return PXVEC3_TO_VEC3(((physx::PxRigidDynamic*)force_actor)->getLinearVelocity());
	else if(actor)
		return PXVEC3_TO_VEC3(((physx::PxRigidDynamic*)actor)->getLinearVelocity());

	return VEC3::Zero;
}

void TCompCollider::setStepOffset(float stepOffset)
{
	_stepOffset = cap_controller->getStepOffset();
	cap_controller->setStepOffset(stepOffset);
}

void TCompCollider::restoreStepOffset()
{
	cap_controller->setStepOffset(_stepOffset);
}

void TCompCollider::setControllerPosition(VEC3 pos)
{
	if (is_capsule_controller)
		cap_controller->setPosition(VEC3_TO_PXEXVEC3(pos));
	else
		box_controller->setPosition(VEC3_TO_PXEXVEC3(pos));
}

void TCompCollider::createController(bool isCapsuleController)
{
	EnginePhysics.createController(*this, isCapsuleController);
}

void TCompCollider::setFootPosition(VEC3 pos)
{
	if (is_capsule_controller)
		cap_controller->setFootPosition(VEC3_TO_PXEXVEC3(pos));
	else
		box_controller->setFootPosition(VEC3_TO_PXEXVEC3(pos));
}

physx::PxU32 TCompCollider::getControllerNbShapes()
{
	physx::PxU32 nshapes;

	if (is_capsule_controller)
		nshapes = cap_controller->getActor()->getNbShapes();
	else
		nshapes = box_controller->getActor()->getNbShapes();

	return nshapes;
}

void TCompCollider::enableBoxController()
{
	TCompTransform* t = get<TCompTransform>();
	VEC3 pos = t->getPosition();
	is_capsule_controller = false;
	actor = box_controller->getActor();
	actor->userData = CHandle(this).asVoidPtr();
	box_controller->setFootPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
	EnginePhysics.setSimulationDisabled(cap_controller->getActor(), true);
	EnginePhysics.setSimulationDisabled(box_controller->getActor(), false);
}

void TCompCollider::enableCapsuleController()
{
	TCompTransform* t = get<TCompTransform>();
	VEC3 pos = t->getPosition();
	is_capsule_controller = true;
	actor = cap_controller->getActor();
	actor->userData = CHandle(this).asVoidPtr();
	cap_controller->setFootPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
	EnginePhysics.setSimulationDisabled(box_controller->getActor(), true);
	EnginePhysics.setSimulationDisabled(cap_controller->getActor(), false);
}
