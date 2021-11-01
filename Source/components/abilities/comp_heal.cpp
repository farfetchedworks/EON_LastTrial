#include "mcv_platform.h"
#include "components/abilities/comp_heal.h"
#include "components/stats/comp_health.h"
#include "components/stats/comp_warp_energy.h"
#include "components/gameplay/comp_game_manager.h"

DECL_OBJ_MANAGER("heal", TCompHeal)

TCompHeal::~TCompHeal()
{

}

void TCompHeal::load(const json& j, TEntityParseContext& ctx)
{
	warp_consumption = j.value("warp_consumption", warp_consumption);
	heal_value = j.value("heal_value", heal_value);
}

void TCompHeal::onEntityCreated()
{
	c_health = get<TCompHealth>();
	assert(c_health.isValid());

	if (CHandle(this).getOwner() == getEntityByName("player")) {
		return;
	}

	// Reduce the enemy's healing amount according to the DDA parameter correction
	// If no GM, it's preloading phase
	if (GameManager)
	{
		TCompGameManager* c_game_manager = GameManager->get<TCompGameManager>();
		heal_value = static_cast<int>(c_game_manager->getAdjustedParameter(TCompGameManager::EParameterDDA::HEAL_AMOUNT, (float)heal_value));
	}
}

void TCompHeal::debugInMenu()
{
	ImGui::DragInt("Warp consumption", &warp_consumption, 1, 0, 50);
}

void TCompHeal::heal()
{
	TCompHealth* health_comp = c_health;
	// health_comp->fillHealth();
	health_comp->increaseHealth(heal_value);
	TCompWarpEnergy* comp_warp = get<TCompWarpEnergy>();

	// enemies don't have warp energy
	if(comp_warp) comp_warp->consumeWarpEnergy(getWarpConsumption());
}
