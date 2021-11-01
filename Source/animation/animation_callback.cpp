#include "mcv_platform.h"
#include "entity/entity.h"
#include "animation_callback.h"
#include "components/messages.h"

CHandle CAnimationCallback::getOwnerEntity(void* userData)
{
	CHandle h_owner;
	h_owner.fromVoidPtr(userData);
	return h_owner;
}

void CAnimationCallback::endCallback(CalCoreAnimation* anim)
{
	if (!anim)
		return;

	for (auto& record : anim->getCallbackList())
	{
		delete record.callback;
	}
	
	anim->getCallbackList().clear();
}