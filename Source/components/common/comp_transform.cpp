#include "mcv_platform.h"
#include "comp_transform.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("transform", TCompTransform)

void TCompTransform::debugInMenu() {
	CTransform::renderInMenu();
}

void TCompTransform::load(const json& j, TEntityParseContext& ctx) {
	CTransform::fromJson(j);

	use_parent_transform = j.value("use_parent_transform", use_parent_transform);

	if (use_parent_transform)
	{
		set(ctx.root_transform.combinedWith(*this));
	}
}

void TCompTransform::set(const CTransform& new_tmx)
{
	*(CTransform*)this = new_tmx;
}


void TCompTransform::renderDebug() {
	//drawAxis(asMatrix());
}

