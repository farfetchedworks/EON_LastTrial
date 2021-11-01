#include "mcv_platform.h"
#include "comp_fluid_simulation.h"

DECL_OBJ_MANAGER("fluid_simulation", TCompFluidSimulation)

void TCompFluidSimulation::load(const json& j, TEntityParseContext& ctx)
{
/*	spawn_prefab = "";
	spawn_prefab = j.value("spawn_prefab", spawn_prefab);
	fall_time = j.value("fall_time", fall_time*/
}

void TCompFluidSimulation::debugInMenu()
{

}

void TCompFluidSimulation::renderDebug()
{

}

void TCompFluidSimulation::onEntityCreated()
{

}

void TCompFluidSimulation::update(float dt)
{

}
