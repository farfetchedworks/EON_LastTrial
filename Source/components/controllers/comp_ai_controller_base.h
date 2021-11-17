#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/controllers/comp_pawn_controller.h"
#include "components/messages.h"
#include "pawn_actions.h"

class TCompAIControllerBase : public TCompPawnController {

    DECL_SIBLING_ACCESS();

protected:
    VEC3 _spawnPoint;
    QUAT _targetRotation;
    
    std::vector<VEC3> waypoints = {};

    bool has_to_rotate = false;                 // If true, the rotation section in the update method will be executed
    bool can_patrol = true;                     // If true, the enemy can follow a set of waypoints
    bool can_observe = false;                   // If true, the enemy will be able to look for Eon depending on the BT
    bool random_patrol = true;                 // If true, the enemy will patrol without the need of waypoints

    void loadCommonProperties(const json& j);
    void debugCommonProperties();
    void placeOnGround();
    void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);

public:
    void onEntityCreated();
    void load(const json& j, TEntityParseContext& ctx);
    void update(float dt);
    void renderDebug();
    void debugInMenu();
    
    VEC3 getMoveDir() { return move_dir; };
    VEC3 getSpawnpoint();
    std::vector<VEC3> getWaypoints() { return waypoints; }

    void rotate(const QUAT delta_rotation);
    void rotate(const float yaw, const float pitch = 0, const float roll = 0);
    void setTargetRotation(QUAT new_target_rot) { _targetRotation = new_target_rot; }

    // Called from the BT to initialize variables such as waypoints and the ability to observe or patrol
    void initProperties();              

    static void registerMsgs() {
      DECL_MSG(TCompAIControllerBase, TMsgRegisterWeapon, onNewWeapon);
      DECL_MSG(TCompAIControllerBase, TMsgAllEntitiesCreated, onAllEntitiesCreated);
    }
};
