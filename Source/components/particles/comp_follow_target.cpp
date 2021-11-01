#include "mcv_platform.h"
#include "comp_follow_target.h"
#include "../bin/data/shaders/constants_particles.h"

DECL_OBJ_MANAGER("particles_follow_target", TCompParticlesFollowTarget);

void TCompParticlesFollowTarget::resolve(const std::string& target_name, VEC3 offset) {
	_target = getEntityByName(target_name);
	assert(_target.isValid());
	_enabled = true;
	_offset = offset;
}

void TCompParticlesFollowTarget::update(float dt) {

	if (!_enabled)
		return;

	CEntity* e = _target;
	if (!e) {
		CHandle(this).getOwner().destroy();
		return;
	}

	TCompBuffers* buffers = get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));
	cte->emitter_owner_position = e->getPosition() + _offset;
	cte->updateFromCPU();
}
