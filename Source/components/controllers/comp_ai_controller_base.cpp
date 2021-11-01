#include "mcv_platform.h"
#include "comp_ai_controller_base.h"
#include "render/draw_primitives.h"
#include "components/common/comp_transform.h"
#include "components/ai/comp_bt.h"
#include "bt/bt_blackboard.h"

// To place enemies on the ground at the beginning
#include "engine.h"
#include "modules/module_physics.h"


DECL_OBJ_MANAGER("ai_controller_base", TCompAIControllerBase)

void TCompAIControllerBase::load(const json& j, TEntityParseContext& ctx)
{
  // Load waypoints or set the current position as the only waypoint
  if (j.count("waypoints")) {
    const json& jwaypoints = j["waypoints"];
    for (const json& jwaypoint : jwaypoints) {
      waypoints.push_back(loadVEC3(jwaypoint));
    }
  }

  random_patrol = j.value("random_patrol", random_patrol);
  can_patrol = j.value("can_patrol", can_patrol);
  can_observe = j.value("can_observe", can_observe);

  sight_radius = j.value("sight_radius", sight_radius);
  hearing_radius = j.value("hearing_radius", hearing_radius);

  // The input of the FOV must be in degrees
  float fov_deg = j.value("fov_deg", 90.f);
  fov_rad = deg2rad(fov_deg);
}

void TCompAIControllerBase::update(float dt)
{
    TCompPawnController::update(dt * speed_multiplier);

    TCompTransform* t = get<TCompTransform>();
    assert(t);

    // Only rotate when necessary
    if (has_to_rotate) {
      if (_targetRotation != t->getRotation()) {
        t->setRotation(dampQUAT(t->getRotation(), _targetRotation, 6.f, dt));
      }
      else {
        has_to_rotate = false;
      }
    }
}

void TCompAIControllerBase::rotate(const QUAT delta_rotation)
{
    TCompTransform* t = get<TCompTransform>();
    assert(t);
    _targetRotation = t->getRotation() * delta_rotation;
    has_to_rotate = true;
}

void TCompAIControllerBase::rotate(const float yaw, const float pitch, const float roll)
{
    rotate(QUAT::CreateFromYawPitchRoll(yaw, pitch, roll));
}

VEC3 TCompAIControllerBase::getSpawnpoint()
{
    return _spawnPoint;
}

void TCompAIControllerBase::placeOnGround()
{
    TCompTransform* c_trans = get<TCompTransform>();
    VHandles v_colliders;
    physx::PxU32 ground_mask = CModulePhysics::FilterGroup::Scenario;
    VEC3 nearest_ground_pos;
  
    // Generate a raycast towards the floor (supposing the enemy has been placed above the ground)
    EnginePhysics.raycast(c_trans->getPosition() + VEC3::Up, VEC3::Down, 99999.f, v_colliders, nearest_ground_pos, ground_mask);
  
    // Just in case the enemy has been placed below the ground
    if (nearest_ground_pos==VEC3::Zero)
      EnginePhysics.raycast(c_trans->getPosition() + VEC3::Up, VEC3::Up, 99999.f, v_colliders, nearest_ground_pos, ground_mask);

    // Place the enemy in the nearest position found
    TCompCollider* c_collider = h_collider;
    c_collider->setFootPosition(nearest_ground_pos);
}

void TCompAIControllerBase::onEntityCreated()
{
    TCompPawnController::onOwnerEntityCreated(CHandle(this).getOwner());
    
    // When the scene is reloaded, entities have already been created, so they must be placed on ground in this method
    placeOnGround();
}

void TCompAIControllerBase::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
    // This has to be done after all antities have been created in case the scenario is created after the enemy
    placeOnGround();
}

void TCompAIControllerBase::initProperties()
{
    // Store the position where enemy was spawned
    TCompTransform* t = get<TCompTransform>();
    _spawnPoint = t->getPosition();

    if (waypoints.size() <= 0) {
      TCompTransform* c_trans = get<TCompTransform>();
      waypoints.push_back(c_trans->getPosition());
    }

    // Set BB variables if applicable
    TCompBT* c_bt = get<TCompBT>();
    CBTBlackboard* enemy_bb = c_bt->getContext()->getBlackboard();
    enemy_bb->setValue<bool>("canObserve", can_observe);
    enemy_bb->setValue<bool>("canPatrol", can_patrol);
    enemy_bb->setValue<bool>("randomPatrol", random_patrol);
}

void TCompAIControllerBase::loadCommonProperties(const json& j) {}

void TCompAIControllerBase::debugCommonProperties()
{
    // Sensing
    ImGui::DragFloat("FOV rad", &fov_rad, 0.1f, 0.f, 3.14f * 2);
    ImGui::DragFloat("Sight Radius", &sight_radius, 2.f, 0.f, 200.f);
}

void TCompAIControllerBase::renderDebug()
{
    TCompTransform* my_transform = get<TCompTransform>();

#ifdef _DEBUG

    // Draw player forward
    VEC3 pos = my_transform->getPosition() + VEC3(0, 1, 0);
    // drawLine(pos, pos + my_transform->getForward() * 1.f, Colors::Orange);
    
    CEntity* e = getEntityByName("player");
    VEC3 playerPos = e->getPosition() + VEC3::Up;

    VEC3 right = normVEC3(playerPos - pos);
    right = VEC3::Transform(right, QUAT::CreateFromAxisAngle(VEC3::Up, (float)M_PI_2));
    drawLine(pos, pos + right * 1.f, Colors::Orange);
    drawLine(pos, pos - right * 1.f, Colors::ShinyBlue);

    // Draw cone
    // drawCone(my_transform, fov_rad, sight_radius);
#endif
}

void TCompAIControllerBase::debugInMenu()
{
  float fov_deg = rad2deg(fov_rad);
  
  ImGui::Checkbox("Random Patrol", &random_patrol);
  ImGui::Checkbox("Can Patrol", &can_patrol);
  ImGui::Checkbox("Can Observe", &can_observe);

  ImGui::SliderFloat("Sight Radius", &sight_radius, 0.f, 1000.f, "%1.f");
  ImGui::SliderFloat("Hearing Radius", &hearing_radius, 0.f, 1000.f, "%1.f");
  ImGui::SliderFloat("Field of View", &fov_deg, 0.f, 100.f, "%.3f");

  fov_rad = deg2rad(fov_deg);
}