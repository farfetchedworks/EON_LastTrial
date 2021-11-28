#include "mcv_platform.h"
#include "module_physics.h"
#include "engine.h"
#include "modules/module_boot.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_tags.h"
#include "components/common/comp_render.h"
#include "audio/physical_material.h"
#include "components/controllers/comp_player_controller.h"
#include "render/meshes/mesh_io.h"
#include "render/draw_primitives.h"
#include "components/stats/comp_health.h"
#include <sstream>

#pragma comment(lib,"PhysX3_x64.lib")
#pragma comment(lib,"PhysX3Common_x64.lib")
#pragma comment(lib,"PhysX3Extensions.lib")
#pragma comment(lib,"PxFoundation_x64.lib")
#pragma comment(lib,"PxPvdSDK_x64.lib")
#pragma comment(lib,"PhysX3CharacterKinematic_x64.lib")
#pragma comment(lib,"PhysX3Cooking_x64.lib")

using namespace physx;

PxDefaultAllocator		gAllocator;
PxDefaultErrorCallback	gErrorCallback;

PxFoundation* gFoundation = NULL;
PxPhysics* gPhysics = NULL;

PxDefaultCpuDispatcher* gDispatcher = NULL;
PxMaterial* gMaterial = NULL;

PxPvd* gPvd = NULL;

PxControllerManager* gControllerManager = nullptr;
physx::PxQueryFilterData CModulePhysics::defaultFilter;


// -----------------------------------------------------
CTransform toTransform(PxTransform pxtrans) {
	CTransform trans;
	trans.setPosition(PXVEC3_TO_VEC3(pxtrans.p));
	trans.setRotation(PXQUAT_TO_QUAT(pxtrans.q));
	return trans;
}

// -----------------------------------------------------
PxTransform toPxTransform(CTransform mcvtrans) {
	PxTransform trans;
	trans.p = VEC3_TO_PXVEC3(mcvtrans.getPosition());
	trans.q = QUAT_TO_PXQUAT(mcvtrans.getRotation());
	return trans;
}

CModulePhysics::FilterGroup CModulePhysics::getFilterByName(const std::string& name)
{
	if (strcmp("player", name.c_str()) == 0) {
		return FilterGroup::Player;
	}
	else if (strcmp("enemy", name.c_str()) == 0) {
		return FilterGroup::Enemy;
	}
	else if (strcmp("projectile", name.c_str()) == 0) {
		return FilterGroup::Projectile;
	}
	else if (strcmp("characters", name.c_str()) == 0) {
		return FilterGroup::Characters;
	}
	else if (strcmp("wall", name.c_str()) == 0) {
		return FilterGroup::Wall;
	}
	else if (strcmp("floor", name.c_str()) == 0) {
		return FilterGroup::Floor;
	}
	else if (strcmp("scenario", name.c_str()) == 0) {
		return FilterGroup::Scenario;
	}
	else if (strcmp("interactable", name.c_str()) == 0) {
		return FilterGroup::Interactable;
	}
	else if (strcmp("interactable_no_collision", name.c_str()) == 0) {
		return FilterGroup::InteractableNoCamCollision;
	}
	else if (strcmp("door", name.c_str()) == 0) {
		return FilterGroup::Door;
	}
	else if (strcmp("effect_area", name.c_str()) == 0) {
		return FilterGroup::EffectArea;
	}
	else if (strcmp("weapon", name.c_str()) == 0) {
		return FilterGroup::Weapon;
	}
	else if (strcmp("prop", name.c_str()) == 0) {
		return FilterGroup::Prop;
	}
	else if (strcmp("trigger", name.c_str()) == 0) {
		return FilterGroup::Trigger;
	}	
	else if (strcmp("invisible_wall", name.c_str()) == 0) {
		return FilterGroup::InvisibleWall;
	}
	else if (strcmp("invisible_wall_enemy", name.c_str()) == 0) {
		return FilterGroup::InvisibleWallEnemy;
	}
	else if (strcmp("none", name.c_str()) == 0) {
		return FilterGroup::None;
	}
	else if (strcmp("all_but_player", name.c_str()) == 0) {
		return FilterGroup::AllButPlayer;
	}
	else if (strcmp("all_but_effect_area", name.c_str()) == 0) {
		return FilterGroup::AllButEffectArea;
	}	
	else if (strcmp("all_but_invisible_wall_enemy", name.c_str()) == 0) {
		return FilterGroup::AllButInvisibleWallEnemy;
	}
	return FilterGroup::All;
}

void CModulePhysics::setupFiltering(PxShape* shape, PxU32 filterGroup, PxU32 filterMask)
{
	PxFilterData filterData;
	filterData.word0 = filterGroup; // word0 = own ID
	filterData.word1 = filterMask;	// word1 = ID mask to filter pairs that trigger a contact callback;
	shape->setSimulationFilterData(filterData);
	shape->setQueryFilterData(filterData);
}

void CModulePhysics::getFilteringFromShape(PxShape* shape, PxU32 &filterGroup, PxU32 &filterMask)
{
	PxFilterData filterData = shape->getSimulationFilterData();
	filterGroup = filterData.word0;
	filterMask = filterData.word1;
}

void CModulePhysics::getFilteringFromAllShapesofActor(PxRigidActor* actor, PxU32 filterGroup [], PxU32 filterMask [])
{
	const PxU32 numShapes = actor->getNbShapes();
	std::vector<PxShape*> shapes;
	shapes.resize(numShapes);
	actor->getShapes(&shapes[0], numShapes);
	for (PxU32 i = 0; i < numShapes; i++) {
		getFilteringFromShape(shapes[i], filterGroup[i], filterMask[i]);
	}
}

void CModulePhysics::setupFilteringOnAllShapesOfActor(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask)
{
	assert(filterGroup != FilterGroup::All);
	const PxU32 numShapes = actor->getNbShapes();
	std::vector<PxShape*> shapes;
	shapes.resize(numShapes);
	actor->getShapes(&shapes[0], numShapes);
	for (PxU32 i = 0; i < numShapes; i++)
		setupFiltering(shapes[i], filterGroup, filterMask);
}

// Reads filters from a string separated with "|"
PxU32 CModulePhysics::readFilterFromJson(std::string mask_string)
{
	PxU32 result_mask = 0;
	std::vector<std::string> mask_names;
	std::stringstream json_string(mask_string);
	
	while (json_string.good()) {
		std::string substr;
		getline(json_string, substr, '|'); //get first string delimited by comma
		result_mask |= getFilterByName(substr);
	}
		
	return result_mask;
	
}

void CModulePhysics::setSimulationDisabled(physx::PxRigidActor* actor, bool disabled)
{
	assert(actor);
	actor->setActorFlag(physx::PxActorFlag::eDISABLE_SIMULATION, disabled);
}

// -----------------------------------------------------
static PxGeometryType::Enum readGeometryType(const json& j) {
	PxGeometryType::Enum geometryType = PxGeometryType::eINVALID;
	if (!j.count("shape"))
		return geometryType;

	std::string jgeometryType = j["shape"];
	if (jgeometryType == "sphere") {
		geometryType = PxGeometryType::eSPHERE;
	}
	else if (jgeometryType == "box") {
		geometryType = PxGeometryType::eBOX;
	}
	else if (jgeometryType == "plane") {
		geometryType = PxGeometryType::ePLANE;
	}
	else if (jgeometryType == "convex") {
		geometryType = PxGeometryType::eCONVEXMESH;
	}
	else if (jgeometryType == "capsule") {
		geometryType = PxGeometryType::eCAPSULE;
	}
	else if (jgeometryType == "trimesh") {
		geometryType = PxGeometryType::eTRIANGLEMESH;
	}
	else {
		dbg("Unsupported shape type in collider component %s", jgeometryType.c_str());
	}
	return geometryType;
}

physx::PxMaterial* CModulePhysics::createMaterial(const VEC3& mat)
{
	return gPhysics->createMaterial(mat.x, mat.y, mat.z);
}

bool CModulePhysics::readShape(PxRigidActor* actor, const json& jcfg)
{
	// Shapes....
	PxGeometryType::Enum geometryType = readGeometryType(jcfg);
	if (geometryType == PxGeometryType::eINVALID)
		return false;

	PxShape* shape = nullptr;
	if (geometryType == PxGeometryType::eBOX)
	{
		VEC3 jhalfExtent = loadVEC3(jcfg, "half_size");
		PxVec3 halfExtent = VEC3_TO_PXVEC3(jhalfExtent);
		shape = gPhysics->createShape(PxBoxGeometry(halfExtent), *gMaterial, true);
	}
	else if (geometryType == PxGeometryType::ePLANE)
	{
		VEC3 jplaneNormal = loadVEC3(jcfg, "normal");
		float jplaneDistance = jcfg.value("distance", 0.f);
		PxVec3 planeNormal = VEC3_TO_PXVEC3(jplaneNormal);
		PxReal planeDistance = jplaneDistance;
		PxPlane plane(planeNormal, planeDistance);
		PxTransform offset = PxTransformFromPlaneEquation(plane);
		shape = gPhysics->createShape(PxPlaneGeometry(), *gMaterial, true);
		shape->setLocalPose(offset);
	}
	else if (geometryType == PxGeometryType::eSPHERE)
	{
		PxReal radius = jcfg.value("radius", 1.f);;
		shape = gPhysics->createShape(PxSphereGeometry(radius), *gMaterial, true);
	}
	else if (geometryType == PxGeometryType::eCAPSULE)
	{
		PxReal radius = jcfg.value("radius", 1.f);
		PxReal half_height = jcfg.value("height", 1.f) * 0.5f;
		PxMaterial* mMaterial = createMaterial(VEC3(0.5f, 0.1f, 0.1f)); // Esto se movera
		shape = gPhysics->createShape(PxCapsuleGeometry(radius, half_height), *mMaterial, true);
	}
	else if (geometryType == PxGeometryType::eCONVEXMESH)
	{
		std::string col_mesh_name = jcfg.value("collision_mesh", "");
		TMeshIO mesh_io;
		CMemoryDataProvider mdp(col_mesh_name.c_str());
		bool is_ok = mesh_io.read(mdp);
		assert(is_ok);

		PxU8* cooked_data = nullptr;
		PxU32 cooked_size = 0;

		// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html

		// Do we have a physics cooked data in the file???
		if (mesh_io.collision_cooked.empty()) {

			PxConvexMeshDesc meshDesc = {};
			meshDesc.points.count = mesh_io.header.num_vertices;
			meshDesc.points.data = mesh_io.vertices.data();
			meshDesc.points.stride = sizeof(PxVec3);

			meshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eDISABLE_MESH_VALIDATION | PxConvexFlag::eFAST_INERTIA_COMPUTATION;
			/*
			// Physics: compute the convex for me
			meshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

			//meshDesc.indices.count = mesh_io.header.num_indices;
			//meshDesc.indices.stride = mesh_io.header.bytes_per_index;
			//meshDesc.indices.data = mesh_io.indices.data();

			//if (mesh_io.header.bytes_per_index == 2)
			//	meshDesc.flags |= PxConvexFlag::e16_BIT_INDICES;

			//// My Mesh is a indexed triangular mesh
			meshDesc.polygons.count = mesh_io.header.num_indices / 3;
			meshDesc.polygons.stride = 3 * mesh_io.header.bytes_per_index;
			meshDesc.polygons.data = mesh_io.indices.data();
			if (mesh_io.header.bytes_per_index == 2)
				meshDesc.flags |= PxConvexFlag::e16_BIT_INDICES;
				*/

						// Find a physics obj to create the cooking
			PxTolerancesScale scale;
			PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION, gPhysics->getFoundation(), PxCookingParams(scale));

			//bool is_valid = cooking->validateConvexMesh(meshDesc);
			//assert(is_valid);

			// Place to store the results of the cooking
			PxDefaultMemoryOutputStream writeBuffer;
			PxConvexMeshCookingResult::Enum result;
			bool status = cooking->cookConvexMesh(meshDesc, writeBuffer, &result);
			assert(status);

			// Num bytes of the resulting cooking process
			cooked_size = writeBuffer.getSize();
			cooked_data = writeBuffer.getData();

			// Transfer the cooked mesh data into the mesh_io, so it's saved and the next time is used
			mesh_io.collision_cooked.resize(cooked_size);
			memcpy(mesh_io.collision_cooked.data(), cooked_data, cooked_size);

			// Regenerate the cmesh so it contains the raw collision mesh + cooked data
			CFileDataSaver fds(col_mesh_name.c_str());
			assert(fds.isValid());
			mesh_io.write(fds);

			cooking->release();
		}

		// the cooked data comes after the raw collision mesh
		cooked_size = (uint32_t)mesh_io.collision_cooked.size();
		cooked_data = mesh_io.collision_cooked.data();

		// 
		assert(cooked_data != nullptr);
		assert(cooked_size > 0);

		PxDefaultMemoryInputData readBuffer(cooked_data, cooked_size);
		PxConvexMesh* convex_mesh = gPhysics->createConvexMesh(readBuffer);
		assert(convex_mesh);

		shape = gPhysics->createShape(PxConvexMeshGeometry(convex_mesh), *gMaterial, true);

		// Now create a render mesh that we will use for debug
		CMesh* render_mesh = new CMesh();
		is_ok = render_mesh->create(mesh_io);
		assert(is_ok);
		char res_name[64];
		sprintf(res_name, "Convex_%p", render_mesh);
		Resources.registerResource(render_mesh, res_name, getClassResourceType<CMesh>());

		shape->userData = render_mesh;
	}
	else if (geometryType == PxGeometryType::eTRIANGLEMESH)
	{
		std::string col_mesh_name = jcfg.value("collision_mesh", "");
		
		TMeshIO mesh_io;
		CMemoryDataProvider mdp(col_mesh_name.c_str());
		bool is_ok = mesh_io.read(mdp);
		assert(is_ok);

		PxU8* cooked_data = nullptr;
		PxU32 cooked_size = 0;

		// Do we have a physics cooked data in the file???
		if (mesh_io.collision_cooked.empty()) {
			
			// No, so we need to cook it
			assert(mesh_io.header.bytes_per_vertex == sizeof(PxVec3));

			PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = mesh_io.header.num_vertices;
			meshDesc.points.data = mesh_io.vertices.data();
			meshDesc.points.stride = sizeof(PxVec3);

			// Physics is using a ccw/cw convention
			meshDesc.flags |= PxMeshFlag::eFLIPNORMALS;

			// My Mesh is a indexed triangular mesh
			meshDesc.triangles.count = mesh_io.header.num_indices / 3;
			meshDesc.triangles.stride = 3 * mesh_io.header.bytes_per_index;
			meshDesc.triangles.data = mesh_io.indices.data();
			if (mesh_io.header.bytes_per_index == 2)
				meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;

			// Find a physics obj to create the cooking
			PxTolerancesScale scale;
			PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION, gPhysics->getFoundation(), PxCookingParams(scale));

			bool is_valid = cooking->validateTriangleMesh(meshDesc);
			// assert(is_valid);

			// Place to store the results of the cooking
			PxDefaultMemoryOutputStream writeBuffer;
			PxTriangleMeshCookingResult::Enum result;
			bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
			assert(status);

			// Num bytes of the resulting cooking process
			cooked_size = writeBuffer.getSize();
			cooked_data = writeBuffer.getData();

			// Transfer the cooked mesh data into the mesh_io, so it's saved and the next time is used
			mesh_io.collision_cooked.resize(cooked_size);
			memcpy(mesh_io.collision_cooked.data(), cooked_data, cooked_size);

			// Regenerate the cmesh so it contains the raw collision mesh + cooked data
			CFileDataSaver fds(col_mesh_name.c_str());
			assert(fds.isValid());
			mesh_io.write(fds);

			cooking->release();
		}

		// the cooked data comes after the raw collision mesh
		cooked_size = (uint32_t)mesh_io.collision_cooked.size();
		cooked_data = mesh_io.collision_cooked.data();

		// 
		assert(cooked_data != nullptr);
		assert(cooked_size > 0);

		PxDefaultMemoryInputData readBuffer(cooked_data, cooked_size);
		PxTriangleMesh* tri_mesh = gPhysics->createTriangleMesh(readBuffer);
		assert(tri_mesh);

		shape = gPhysics->createShape(PxTriangleMeshGeometry(tri_mesh), *gMaterial, true);

		// Now create a render mesh that we will use for debug
		CMesh* render_mesh = new CMesh();
		is_ok = render_mesh->create(mesh_io);
		assert(is_ok);
		char res_name[64];
		sprintf(res_name, "Physics_%p", render_mesh);
		Resources.registerResource(render_mesh, res_name, getClassResourceType<CMesh>());

		shape->userData = render_mesh;
	}
	else
	{
		fatal("not implemented yet");
	}

	if (jcfg.value("trigger", false))
	{
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	}

	// Shape offset
	if (jcfg.count("offset")) {
		CTransform trans;
		trans.fromJson(jcfg["offset"]);
		shape->setLocalPose(toPxTransform(trans));
	}

	PxU32 filter_group;
	PxU32 filter_mask;

	// Read filters separated with a "|" character
	if (jcfg.find("group") != jcfg.end()) {
		filter_group = readFilterFromJson(jcfg["group"]);
	}
	else {
		filter_group = getFilterByName("scenario");
	}
	if (jcfg.find("mask") != jcfg.end()) {
		filter_mask = readFilterFromJson(jcfg["mask"]);
	}
	else {
		filter_mask = getFilterByName("all");
	}

	setupFiltering(
		shape,
		filter_group,
		filter_mask
	);

	actor->attachShape(*shape);
	shape->release();
	return true;
}

PxRigidActor* CModulePhysics::createController(TCompCollider& comp_collider, bool isCapsuleController)
{
	PxRigidActor* actor = nullptr;
	PxRigidActor* c_actor = nullptr;
	PxRigidActor* b_actor = nullptr;
	//comp_collider.releaseController();

	{
		c_actor = createCapsuleController(comp_collider);
		b_actor = createBoxController(comp_collider);
		actor = isCapsuleController ? c_actor : b_actor;
		comp_collider.actor = actor;
	}

	if (c_actor) {
		gScene->addActor(*c_actor);
		comp_collider.c_actor = c_actor;
		CHandle h_collider(&comp_collider);
		comp_collider.c_actor->userData = h_collider.asVoidPtr();
	}

	if (b_actor) {
		gScene->addActor(*b_actor);
		comp_collider.b_actor = b_actor;
		CHandle h_collider(&comp_collider);
		comp_collider.b_actor->userData = h_collider.asVoidPtr();
	}

	return actor;
}

// TODO Refactoring
PxRigidActor* CModulePhysics::createCapsuleController(TCompCollider& comp_collider) 
{
	physx::PxController* controller;
	const json& jconfig = comp_collider.jconfig;

	TCompTransform* trans = comp_collider.get<TCompTransform>();
	VEC3 pos = trans->getPosition();
	
	// Create capsule controller
	PxRigidActor* c_actor = nullptr;
	{
		PX_ASSERT(desc.mType == PxControllerShapeType::eCAPSULE);

		PxCapsuleControllerDesc capsuleDesc;
		capsuleDesc.height = jconfig.value("height", 1.f);
		capsuleDesc.radius = jconfig.value("radius", 1.f);
		capsuleDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
		capsuleDesc.material = gMaterial;
		capsuleDesc.stepOffset = jconfig.value("stepOffset", 0.25f);
		capsuleDesc.slopeLimit = jconfig.value("slopeLimit", 0.707f);
		//capsuleDesc.contactOffset = 0.1f;
		//capsuleDesc.reportCallback = &customHitController; // 

		controller = static_cast<PxCapsuleController*>(gControllerManager->createController(capsuleDesc));
		PxCapsuleController* ctrl = static_cast<PxCapsuleController*>(controller);

		PX_ASSERT(ctrl);
		
		ctrl->setFootPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
		//ctrl->setContactOffset(0.1f);
		c_actor = ctrl->getActor();
		comp_collider.cap_controller = ctrl;
		comp_collider.c_actor = c_actor;
	}

	//comp_collider.actor = comp_collider.disabled_box_controller ? c_actor : b_actor;

	// Allow multiple filters in groups and masks for controllers
	PxU32 filter_group;
	PxU32 filter_mask;
	if (jconfig.find("group") != jconfig.end()) {
		filter_group = readFilterFromJson(jconfig["group"]);
	}
	else {
		filter_group = getFilterByName("characters");
	}
	if (jconfig.find("mask") != jconfig.end()) {
		filter_mask = readFilterFromJson(jconfig["mask"]);
	}
	else {
		filter_mask = getFilterByName("all");
	}

	setupFilteringOnAllShapesOfActor(c_actor,
		filter_group,
		filter_mask
	);

	return c_actor;
}

PxRigidActor* CModulePhysics::createBoxController(TCompCollider& comp_collider)
{
	physx::PxController* controller;
	const json& jconfig = comp_collider.jconfig;

	TCompTransform* trans = comp_collider.get<TCompTransform>();
	VEC3 pos = trans->getPosition();

	// Create box controller
	PxRigidActor* b_actor = nullptr;
	{

		PX_ASSERT(desc.mType == PxControllerShapeType::eBOX);

		physx::PxBoxControllerDesc boxDesc;
		VEC3 box_size = VEC3::Zero;

		if (jconfig.count("size") > 0)
			box_size = loadVEC3(jconfig["size"]);

		physx::PxVec3 size = physx::PxVec3(box_size.x, box_size.y, box_size.z);

		boxDesc.halfHeight = size.y;
		boxDesc.halfSideExtent = size.x;
		boxDesc.halfForwardExtent = size.z;
		boxDesc.material = gMaterial;
		boxDesc.stepOffset = 0.01f;
		boxDesc.slopeLimit = 0.01f;
		/*boxDesc.stepOffset = jconfig.value("stepOffset", 0.25f);
		boxDesc.slopeLimit = jconfig.value("slopeLimit", 0.707f);*/
		/*cDesc->material = gMaterial;
		cDesc->contactOffset = 0.0f;
		cDesc->stepOffset = jconfig.value("stepOffset", 0.25f);*/
		controller = static_cast<PxBoxController*>(gControllerManager->createController(boxDesc));
		PxBoxController* ctrl = static_cast<PxBoxController*>(controller);

		PX_ASSERT(ctrl);
		if (!ctrl) return b_actor;
		ctrl->setFootPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
		//ctrl->setContactOffset(0.1f);
		b_actor = ctrl->getActor();
		comp_collider.box_controller = ctrl;
		b_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
		comp_collider.b_actor = b_actor;
	}

	//comp_collider.actor = comp_collider.disabled_box_controller ? c_actor : b_actor;

	// Allow multiple filters in groups and masks for controllers
	PxU32 filter_group;
	PxU32 filter_mask;
	if (jconfig.find("group") != jconfig.end()) {
		filter_group = readFilterFromJson(jconfig["group"]);
	}
	else {
		filter_group = getFilterByName("characters");
	}
	if (jconfig.find("mask") != jconfig.end()) {
		filter_mask = readFilterFromJson(jconfig["mask"]);
	}
	else {
		filter_mask = getFilterByName("all");
	}

	setupFilteringOnAllShapesOfActor(b_actor,
		filter_group,
		filter_mask
	);

	return b_actor;
}

void CModulePhysics::createActor(TCompCollider& comp_collider)
{
	const json& jconfig = comp_collider.jconfig;

	TCompTransform* c = comp_collider.get<TCompTransform>();
	PxTransform transform = toPxTransform(*c);

	PxRigidActor* actor = nullptr;
	
	bool is_controller = jconfig.count("controller") > 0;

	bool is_force_actor = comp_collider.cap_controller != nullptr  || comp_collider.box_controller != nullptr;

	// Controller or physics based?
	if (is_controller) 
	{
		comp_collider.is_capsule_controller = jconfig.value("disabledBoxController", true);
		createController(comp_collider, comp_collider.is_capsule_controller);
	}
	else
	{
		// Dynamic or static actor?
		bool isDynamic = jconfig.value("dynamic", false);
		if (isDynamic)
		{
			PxRigidDynamic* dynamicActor = gPhysics->createRigidDynamic(transform);
			PxReal density = jconfig.value("density", 1.f);
			PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);

			if(jconfig.count("mass"))
				dynamicActor->setMass((PxReal)jconfig["mass"]);
			PxReal maxContactImpulsedensity = jconfig.value("max_impulse", (PxReal)PX_MAX_F32);
			dynamicActor->setMaxContactImpulse(maxContactImpulsedensity);

			actor = dynamicActor;

			if (jconfig.value("kinematic", false))
				dynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		}
		else
		{
			PxRigidStatic* static_actor = gPhysics->createRigidStatic(transform);
			actor = static_actor;
		}
	}

	// The capsule was already loaded as part of the controller
	if (!is_controller) {
		// if shapes exists, try to read as an array of shapes
		if (jconfig.count("shapes")) {
			const json& jshapes = jconfig["shapes"];
			for (const json& jshape : jshapes)
				readShape(actor, jshape);
		}
		// Try to read shape directly (general case...)
		else {
			readShape(actor, jconfig);
		}

		gScene->addActor(*actor);

		CHandle h_collider(&comp_collider);
		actor->userData = h_collider.asVoidPtr();

		if (!is_force_actor) {
			comp_collider.actor = actor;
		}
		else {
			comp_collider.force_actor = actor;
		}
	}
}


void CModulePhysics::removeActors(TCompCollider& comp_collider)
{
	if (comp_collider.actor)
		gScene->removeActor(*comp_collider.actor);

	if (comp_collider.c_actor)
		gScene->removeActor(*comp_collider.c_actor);

	if (comp_collider.b_actor)
		gScene->removeActor(*comp_collider.b_actor);

	gScene->flushQueryUpdates();
}

// ------------------------------------------------------------------
PxFilterFlags CustomFilterShader(
	PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize
)
{
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
	{
		if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		}
		else {
			pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | 
				PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
				PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}
		return PxFilterFlag::eDEFAULT;
	}
	return PxFilterFlag::eSUPPRESS;
}

bool CModulePhysics::start()
{
	gFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, gAllocator, gErrorCallback);

	// disable printing errors 
	gFoundation->setErrorLevel(PxErrorCode::eNO_ERROR);

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
	if (!PxInitExtensions(*gPhysics, gPvd))
		fatal("PxInitExtensions failed!");

	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = CustomFilterShader;
	sceneDesc.flags = PxSceneFlag::eENABLE_ACTIVE_ACTORS | PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS | PxSceneFlag::eENABLE_KINEMATIC_PAIRS;
	gScene = gPhysics->createScene(sceneDesc);

	// Register to the callbacks
	gScene->setSimulationEventCallback(&customSimulationEventCallback);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		//pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		//pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	gControllerManager = PxCreateControllerManager(*gScene);

	return true;
}

void CModulePhysics::stop()
{
	gControllerManager->release();
	gMaterial->release();
	gScene->release();

	gDispatcher->release();
	gPhysics->release();

	PxPvdTransport* transport = gPvd->getTransport();
	gPvd->release();
	transport->release();

	gFoundation->release();
}

void CModulePhysics::update(float dt) {	

	if (!Boot.ready())
		return;

	gScene->simulate(dt);
	gScene->fetchResults(true);

	PxU32 nbActorsOut = 0;
	PxActor** activeActors = gScene->getActiveActors(nbActorsOut);
	for (unsigned int i = 0; i < nbActorsOut; ++i)
	{
		PxRigidActor* rigidActor = ((PxRigidActor*)activeActors[i]);
		assert(rigidActor);
		CHandle h_collider;
		h_collider.fromVoidPtr(rigidActor->userData);
		TCompCollider* c_collider = h_collider;
		if (c_collider != nullptr)
		{

			if (c_collider->force_actor == activeActors[i]) {
				continue;
			}
			
			TCompTransform* c = c_collider->get<TCompTransform>();
			
			if (c == nullptr) continue;
			assert(c);

			if (c_collider->cap_controller || c_collider->box_controller)
			{
				PxExtendedVec3 pxpos_ext = c_collider->getFootPosition();
				c->setPosition(VEC3((float)pxpos_ext.x, (float)pxpos_ext.y, (float)pxpos_ext.z));
			}
			else
			{
				VEC3 scale = c->getScale();
				PxTransform PxTrans = rigidActor->getGlobalPose();
				c->set(toTransform(PxTrans));
				c->setScale(scale);
			}
		}
	}
}

void CModulePhysics::renderInMenu() {
}

void CModulePhysics::render() {
}

void CModulePhysics::renderDebug() {

#ifdef _DEBUG
	for (auto& line : _debugLines) {

		line.timer += Time.delta;

		if (line.timer < line.duration) {
			if (line.has_hit) {
				drawLine(line.origin, line.hit, line.hit_color);
				drawLine(line.hit, line.end, line.end_color);
			}
			else {
				drawLine(line.origin, line.end, line.hit_color);
			}
		}
		else {
			line.remove = true;
		}
	}

	auto eraseIt = std::remove_if(begin(_debugLines), end(_debugLines), [](const DBG_Line& l) {return l.remove; });
	_debugLines.erase(eraseIt, end(_debugLines));
#endif
}


/*
	Query Pre filters
*/

// Pre filter for touching hits
PxQueryHitType::Enum CModulePhysics::CustomQueryFilterCallback::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
{
	const physx::PxFilterData& filterData1 = shape->getQueryFilterData();

	if ((filterData.word0 & filterData1.word1) && (filterData1.word0 & filterData.word1))
	{
		return PxQueryHitType::eTOUCH;								// Returns all touches. To consider only one, use eBLOCK (in this case, consider using another QueryFilterCallback)
	}
	return PxQueryHitType::eNONE;
}

// Pre filter for blocking hits
PxQueryHitType::Enum CModulePhysics::BlockingQueryFilterCallback::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
{
	const physx::PxFilterData& filterData1 = shape->getQueryFilterData();

	if ((filterData.word0 & filterData1.word1) && (filterData1.word0 & filterData.word1))
	{
		return PxQueryHitType::eBLOCK;								// Returns all touches. To consider only one, use eBLOCK (in this case, consider using another QueryFilterCallback)
	}
	return PxQueryHitType::eNONE;
}

void CModulePhysics::CustomSimulationEventCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	for (PxU32 i = 0; i < nbPairs; i++)
	{
		const PxContactPair& cp = pairs[i];

		if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			// ignore pairs when shapes have been deleted
			if (pairs[i].flags & (PxContactPairFlag::eREMOVED_SHAPE_0 | PxContactPairFlag::eREMOVED_SHAPE_1))
				continue;


			CHandle h_comp;
			h_comp.fromVoidPtr(pairHeader.actors[0]->userData);

			CHandle h_comp_other;
			h_comp_other.fromVoidPtr(pairHeader.actors[1]->userData);

			CEntity * e = h_comp.getOwner();
			CEntity * e_other = h_comp_other.getOwner();

			if (e && e_other)
			{
				TMsgEntityOnContact msg;

				// Notify the trigger someone entered
				msg.h_entity = h_comp_other.getOwner();
				e->sendMsg(msg);

				// Notify that someone he entered in a trigger
				msg.h_entity = h_comp.getOwner();
				e_other->sendMsg(msg);
			}
		}
		else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			// ignore pairs when shapes have been deleted
			if (pairs[i].flags & (PxContactPairFlag::eREMOVED_SHAPE_0 | PxContactPairFlag::eREMOVED_SHAPE_1))
				continue;

			CHandle h_comp;
			h_comp.fromVoidPtr(pairHeader.actors[0]->userData);

			CHandle h_comp_other;
			h_comp_other.fromVoidPtr(pairHeader.actors[1]->userData);

			CEntity* e = h_comp.getOwner();
			CEntity* e_other = h_comp_other.getOwner();

			if (e && e_other)
			{
				TMsgEntityOnContactExit msg;

				msg.h_entity = h_comp_other.getOwner();
				e->sendMsg(msg);

				msg.h_entity = h_comp.getOwner();
				e_other->sendMsg(msg);
			}
		}
		else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
		{
			// dbg("CONTACT PERSISTS\n");
		}
	}
}

void CModulePhysics::CustomSimulationEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
	for (PxU32 i = 0; i < count; i++)
	{
		// ignore pairs when shapes have been deleted
		if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			continue;

		CHandle h_comp_trigger;
		h_comp_trigger.fromVoidPtr(pairs[i].triggerActor->userData);

		CHandle h_comp_other;
		h_comp_other.fromVoidPtr(pairs[i].otherActor->userData);

		if (!h_comp_trigger.isValid() || !h_comp_other.isValid())
			return;

		CEntity * e_trigger = h_comp_trigger.getOwner();
		CEntity * e_other = h_comp_other.getOwner();

		if (pairs[i].status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			TMsgEntityTriggerEnter msg;

			// Notify the trigger someone entered
			msg.h_entity = h_comp_other.getOwner();
			e_trigger->sendMsg(msg);
		}
		else if (pairs[i].status == PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			TMsgEntityTriggerExit msg;

			// Notify the trigger someone exit
			msg.h_entity = h_comp_other.getOwner();
			e_trigger->sendMsg(msg);
		}
	}
}

// To make CCTs always collide - and -slide against each other, simply return true.
// To make CCTs always move freely through each other, simply return false.
bool CModulePhysics::CustomCharacterControllerFilter::filter(const physx::PxController& a, const physx::PxController& b)
{
	std::string name_a, name_b;

	{
		CHandle h_collider;
		h_collider.fromVoidPtr(a.getActor()->userData);

		CEntity* owner = h_collider.getOwner();
		name_a = owner->getName();
		TCompCollider* c_collider = h_collider;
		CHandle h_health = c_collider->get<TCompHealth>();
		
		if (!h_health.isValid())
			return true;

		TCompHealth* c_health = h_health;

		if (c_collider->actor != a.getActor())
			return false;

		if (c_health->isDead())
			return false;

	}
	
	{
		CHandle h_collider;
		h_collider.fromVoidPtr(b.getActor()->userData);

		CEntity* owner = h_collider.getOwner();
		name_b = owner->getName();
		TCompCollider* c_collider = h_collider;
		CHandle h_health = c_collider->get<TCompHealth>();

		if (!h_health.isValid())
			return true;

		TCompHealth* c_health = h_health;

		if (c_collider->actor != b.getActor())
			return false;

		if (c_health->isDead())
			return false;
	}
	
	if (name_a == name_b) return false;

	return true;
}

//  Called when current controller hits a shape.
void CModulePhysics::CustomHitController::onShapeHit(const PxControllerShapeHit& hit)
{
	if (hit.shape->getSimulationFilterData().word0 != FilterGroup::Scenario) {

		CHandle h_comp;
		h_comp.fromVoidPtr(hit.controller->getActor()->userData);

		CHandle h_comp_other;
		h_comp_other.fromVoidPtr(hit.actor->userData);

		CEntity* e = h_comp.getOwner();
		CEntity* e_other = h_comp_other.getOwner();

		if (e && e_other)
		{
			TMsgEntityOnContact msg;
			std::string name = e_other->getName();

			// Notify the trigger someone entered 
			msg.h_entity = h_comp_other.getOwner();
			e->sendMsg(msg);

			// Notify that someone he entered in a trigger
			msg.h_entity = h_comp.getOwner();
			e_other->sendMsg(msg);
		}

		// dbg("onShapeHit\n");
	}
}

// brief Called when current controller hits another controller.
void CModulePhysics::CustomHitController::onControllerHit(const PxControllersHit& hit)
{
	CHandle h_comp;
	h_comp.fromVoidPtr(hit.controller->getActor()->userData);

	CHandle h_comp_other;
	h_comp_other.fromVoidPtr(hit.other->getActor()->userData);

	CEntity* e = h_comp.getOwner();
	CEntity* e_other = h_comp_other.getOwner();

	if (e && e_other)
	{
		TMsgEntityOnContact msg;

		// Notify the trigger someone entered
		msg.h_entity = h_comp_other.getOwner();
		e->sendMsg(msg);

		// Notify that someone he entered in a trigger
		msg.h_entity = h_comp.getOwner();
		e_other->sendMsg(msg);
	}

	// dbg("onControllerHit\n");
}

// brief Called when current controller hits a user-defined obstacle.
void CModulePhysics::CustomHitController::onObstacleHit(const PxControllerObstacleHit& hit)
{
	// dbg("onObstacleHit\n");
}

/*
	Generate an overlap sphere in a position with a radius, detecting only colliders in the hitmask (detects all by default)
	Returns true if it hit anything
	Fills the vector "colliders" with the handle of the colliders which were detected in the hit
*/
bool CModulePhysics::overlapSphere(VEC3 position, float radius, VHandles& colliders, PxU32 hitMask)
{
	physx::PxQueryFilterData fd;
	
	// To detect multiple touches, create an Overlap Buffer which will have Overlap Hits
	const int buffer_size = 16;									
	physx::PxOverlapHit hitBuffer[buffer_size];
	physx::PxOverlapBuffer overlapHitsBuffer (hitBuffer, buffer_size);

	// Add flags and filter groups to the query filter data
	fd.flags |= physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER;		// Filter flags must contain PREFILTER to call the prefilter method of the customQueryFilterCallback object
	fd.data.word0 = FilterGroup::All;															// word0 (the object type of the overlap shape) should be "all" because we want all actors to detect the shape
	fd.data.word1 = hitMask;																											// word1 is the mask of the actors with whom the shape will overlap

	// Create the geometry
	physx::PxSphereGeometry overlapShape = physx::PxSphereGeometry(radius);				// [in] shape to test for overlaps. Before it was "physx::PxGeometry overlapShape" but it doesn't create a sphere properly!!!
	physx::PxTransform shapePose(VEC3_TO_PXVEC3(position));

	// overlap receiving the results in OverlapHitsBuffer using the "PreFilter" method defined by the customQueryFilterCallback
	bool status = gScene->overlap(overlapShape, shapePose, overlapHitsBuffer, fd, &customQueryFilterCallback);	

	// If there is a hit, obtain the collider handle of each hit. These hits are considered as "Touches"
	if (status) {
		for (unsigned int i = 0; i < overlapHitsBuffer.nbTouches; i++) {

			const physx::PxOverlapHit& overlapHit = overlapHitsBuffer.getAnyHit(i);
			CHandle h_collider;
			h_collider.fromVoidPtr(overlapHit.actor->userData);

			colliders.push_back(h_collider);
		}
	}
	return status;

}

const CPhysicalMaterial* CModulePhysics::getPhysicalMaterialOfFloor(CHandle actor, VEC3& floor_hit)
{
	CEntity* e_actor = actor;

	// Cast a ray downwards to check whats the material the actor is stepping on
	VHandles colliders;
	physx::PxU32 layerMask = CModulePhysics::FilterGroup::Floor;

	bool hasHit = raycast(e_actor->getPosition() + VEC3::Up, VEC3::Down, 5.f, colliders, floor_hit, layerMask);

	if (!hasHit)
		return nullptr;

	CHandle h_hit_entity = colliders[0].getOwner();
	CEntity* e_hit_entity = h_hit_entity;

	TCompRender* t_render = e_hit_entity->get<TCompRender>();

	if (!t_render || t_render->draw_calls.size() <= 0)
		return nullptr;

	// Get the physical material
	const CMaterial* contact_mat = t_render->draw_calls[0].material;
	return contact_mat->getPhysicalMaterial();
}

/*
	Generate a raycast in a position following the direction "dir" with a distance, detecting only colliders in the hitmask (detects all by default)
	Returns true if it hit anything
	Fills the vector "raycastHits" with information about each hit as a PxRaycastHit
	If GetClosestHit is True, it will return the first blocking hit (by default is false)
*/
bool CModulePhysics::raycast(const VEC3& origin, const VEC3& dir, float distance, std::vector<physx::PxRaycastHit>& raycastHits, PxU32 hitMask, bool getClosestHit, bool render_debug)
{
	physx::PxQueryFilterData fd;

	// To detect multiple touches, create a raycast Buffer which will have raycast Hits
	const int buffer_size = 10;
	physx::PxRaycastHit hitBuffer[buffer_size];
	physx::PxRaycastBuffer raycastHitsBuffer(hitBuffer, buffer_size);

	// If the user needs the closest hit, then it will perform a blocking query filter. Otherwise, it will obtain all touches
	physx::PxQueryFilterCallback* filterCallback;
	if (getClosestHit)
		filterCallback = &blockingQueryFilterCallback;
	else
		filterCallback = &customQueryFilterCallback;

	// Add flags and filter groups to the query filter data
	fd.flags |= physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::ePREFILTER;		// Filter flags must contain PREFILTER to call the prefilter method of the customQueryFilterCallback object
	fd.data.word0 = FilterGroup::All;																														// word0 (the object type of the raycast) should be "all" because we want all actors to detect the shape
	fd.data.word1 = hitMask;																																										// word1 is the mask of the actors with whom the raycast will hit

	// generate the raycast receiving the results in RaycastHitsBuffer using the "PreFilter" method defined by the customQueryFilterCallback
	bool status = gScene->raycast(
		VEC3_TO_PXVEC3(origin),
		VEC3_TO_PXVEC3(dir),
		(PxReal)(distance),
		raycastHitsBuffer,
		PxHitFlags(PxHitFlag::eDEFAULT),
		fd,
		filterCallback);
	
	// Check if there is a hit
	if (status) {

		// If the user wanted the closest hit, return the block member from the raycastHitsBuffer
		if (getClosestHit) {
			raycastHits.push_back(raycastHitsBuffer.block);
		}
		else {		// obtain each raycast hit. These hits are considered as "Touches"
			for (unsigned int i = 0; i < raycastHitsBuffer.nbTouches; i++) {
			
				raycastHits.push_back(raycastHitsBuffer.getAnyHit(i));
			}
		}
	}

	if (render_debug)
	{
#ifdef _DEBUG
		DBG_Line line;
		line.origin = origin;
		line.end = origin + dir * distance;

		if (!raycastHits.empty()) {
			line.has_hit = true;
			line.hit = PXVEC3_TO_VEC3(raycastHits[0].position);
		}

		_debugLines.push_back(line);
#endif
	}

	return status;

}

bool CModulePhysics::sweep(PxTransform initial_pose, const VEC3& dir, float distance, const PxGeometry &shape, std::vector<physx::PxSweepHit>& sweepHits, physx::PxU32 hitMask, bool getClosestHit, bool render_debug)
{
	physx::PxQueryFilterData fd;

	// To detect multiple touches, create a raycast Buffer which will have raycast Hits
	const int buffer_size = 10;
	physx::PxSweepHit hitBuffer[buffer_size];
	physx::PxSweepBuffer sweepHitsBuffer(hitBuffer, buffer_size);

	// If the user needs the closest hit, then it will perform a blocking query filter. Otherwise, it will obtain all touches
	physx::PxQueryFilterCallback* filterCallback;
	if (getClosestHit)
		filterCallback = &blockingQueryFilterCallback;
	else
		filterCallback = &customQueryFilterCallback;

	// Add flags and filter groups to the query filter data
	fd.flags |= physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::ePREFILTER;		// Filter flags must contain PREFILTER to call the prefilter method of the customQueryFilterCallback object
	fd.data.word0 = FilterGroup::All;																				// word0 (the object type of the raycast) should be "all" because we want all actors to detect the shape
	fd.data.word1 = hitMask;																						// word1 is the mask of the actors with whom the raycast will hit

	// generate the raycast receiving the results in RaycastHitsBuffer using the "PreFilter" method defined by the customQueryFilterCallback
	bool status = gScene->sweep(
		shape,
		initial_pose,
		VEC3_TO_PXVEC3(dir),
		(PxReal)(distance),
		sweepHitsBuffer,
		PxHitFlags(PxHitFlag::eDEFAULT),
		fd,
		filterCallback);

	// Check if there is a hit
	if (status) {

		// If the user wanted the closest hit, return the block member from the raycastHitsBuffer
		if (getClosestHit) {
			sweepHits.push_back(sweepHitsBuffer.block);
		}
		else {		// obtain each raycast hit. These hits are considered as "Touches"
			for (unsigned int i = 0; i < sweepHitsBuffer.nbTouches; i++) {

				sweepHits.push_back(sweepHitsBuffer.getAnyHit(i));
			}
		}
	}

	if (render_debug)
	{
#ifdef _DEBUG
		DBG_Line line;
		line.origin = PXVEC3_TO_VEC3(initial_pose.p);
		line.end = line.origin + dir * distance;

		if (!sweepHits.empty()) {
			line.has_hit = true;
			line.hit = PXVEC3_TO_VEC3(sweepHits[0].position);
		}

		_debugLines.push_back(line);
#endif
	}

	return status;
}

/*
	Generate a raycast in a position following the direction "dir" with a distance, detecting only colliders in the hitmask (detects all by default)
	Returns true if it hit anything
	Fills the vector "colliders" with the handle of the colliders which were detected in the hit
	If GetClosestHit is True, it will return the first blocking hit (by default is false)
*/
bool CModulePhysics::raycast(const VEC3& origin, const VEC3& dir, float distance, VHandles& colliders, PxU32 hitMask, bool getClosestHit, bool render_debug, bool orderByDistance)
{
	std::vector<physx::PxRaycastHit> raycastHits;
	bool status = raycast(origin, dir, distance, raycastHits, hitMask, getClosestHit);

	if (orderByDistance) {
		struct {
			bool operator()(physx::PxRaycastHit a, physx::PxRaycastHit b) const { return a.distance < b.distance; }
		} less_distance;
		std::sort(raycastHits.begin(), raycastHits.end(), less_distance);
	}
		

	// Check if there is a hit
	if (status) {

		// If the user wanted the closest hit, return the first hit (which is the blocking hit)
		if (getClosestHit) {
			CHandle h_collider;
			h_collider.fromVoidPtr(raycastHits[0].actor->userData);
			colliders.push_back(h_collider);
		}
		else {		// obtain the collider handle of each hit
			for (unsigned int i = 0; i < raycastHits.size(); i++) {

				const physx::PxRaycastHit& overlapHit = raycastHits[i];
				CHandle h_collider;
				h_collider.fromVoidPtr(overlapHit.actor->userData);

				colliders.push_back(h_collider);
			}
		}

		if (render_debug)
		{
			DBG_Line line;
			line.origin = origin;
			line.has_hit = true;
			line.hit = PXVEC3_TO_VEC3(raycastHits[0].position);
			line.end = origin + dir * distance;
			_debugLines.push_back(line);
		}
	}

	return status;
}

bool CModulePhysics::raycast(const VEC3& origin, const VEC3& dir, float distance, VHandles& colliders, VEC3& first_hit, PxU32 hitMask, bool render_debug)
{
	std::vector<physx::PxRaycastHit> raycastHits;
	bool status = raycast(origin, dir, distance, raycastHits, hitMask, true, render_debug);

	// Check if there is a hit
	if (status) {

		CHandle h_collider;
		h_collider.fromVoidPtr(raycastHits[0].actor->userData);
		colliders.push_back(h_collider);
		first_hit = PXVEC3_TO_VEC3(raycastHits[0].position);

		if (render_debug)
		{
			DBG_Line line;
			line.origin = origin;
			line.has_hit = true;
			line.hit = first_hit;
			line.end = origin + dir * distance;
			_debugLines.push_back(line);
		}
	}

	return status;
}