#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"

struct TCompFocusController : public TCompBase
{
    DECL_SIBLING_ACCESS()

    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void update(float dt);

    void debugInMenu();
    void renderDebug();

    void enable(CHandle target_entity);
    void disable();

private:

    bool _enabled = false;
    float _intensity = 0.0f;

    CHandle _target_entity;

};
