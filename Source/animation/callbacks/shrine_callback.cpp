#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "modules/game/module_player_interaction.h"
#include "animation/animation_callback.h"
#include "components/messages.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/gameplay/comp_shrine.h"

struct onUseShrineCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		PlayerInteraction.setPlaying(false);

		TCompGameManager* comp_gm = GameManager->get<TCompGameManager>();
		comp_gm->respawnEnemies();

		// Always restore
		CEntity* owner = getOwnerEntity(userData);
		assert(owner);
		TCompShrine* shrine = owner->get<TCompShrine>();
		shrine->restorePlayer();
	}
};

REGISTER_CALLBACK("onShrineActivated", onUseShrineCallback)