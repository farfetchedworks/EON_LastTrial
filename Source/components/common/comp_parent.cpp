#include "mcv_platform.h"
#include "comp_parent.h"
#include "components/common/comp_collider.h"

DECL_OBJ_MANAGER("parent_of", TCompParent)

TCompParent::~TCompParent() {
	for (auto h : children)
		h.destroy();
}

bool TCompParent::delChild(CHandle h_curr_child) {
	int idx = 0;
	for (auto h : children) {
		if (h == h_curr_child) {
			// Remove from my vector
			children.erase(children.begin() + idx);
			// Tell the handle, I'm no longer his parent
			h_curr_child.setOwner(CHandle());

			// There should be no more copies of h_curr_child in the vector
			assert(std::find(children.begin(), children.end(), h_curr_child) == children.end());

			return true;
		}
		++idx;
	}
	return false;
}

void TCompParent::addChild(CHandle h_new_child) {

	if (!h_new_child.isValid())
		return;

	// Confirm the handle, if valid, represents an entity
	assert(h_new_child.getType() == getObjectManager<CEntity>()->getType());

	CHandle h_my_parent = CHandle(this).getOwner();
	assert(h_my_parent.isValid());

	// unbind from current parent if exists
	CHandle h_curr_parent = h_new_child.getOwner();
	if (h_curr_parent.isValid()) {

		// I'm already your parent
		if (h_curr_parent == h_my_parent)
			return;

		CEntity* e_curr_parent = h_curr_parent;
		assert(e_curr_parent);
		TCompParent* c_curr_parent = e_curr_parent->get<TCompParent>();
		if (c_curr_parent)
			c_curr_parent->delChild(h_new_child);
	}

	// Ensure it's not already my child 
	assert(std::find(children.begin(), children.end(), h_new_child) == children.end());

	h_new_child.setOwner(h_my_parent);
	children.push_back(h_new_child);
}

CHandle TCompParent::getChildByName(const std::string& name)
{
	for (auto h : children) {
		CEntity* e = h;
		if (std::string(e->getName()) == name)
			return h;
	}

	return CHandle();
}

void TCompParent::forEachChild(std::function<void(CHandle)> fn)
{
	for (auto h : children) {
		fn(h);
	}
}

void TCompParent::disableChildCollider()
{
	for (auto h : children) {
		CEntity* e = h;
		CHandle h_collider = e->get<TCompCollider>();
		// disable collisions with the player and enemies
		if (h_collider.isValid())
			static_cast<TCompCollider*>(h_collider)->setGroupAndMask("none", "none");
	}
}

void TCompParent::debugInMenu() {
	for (auto h : children)
		h.debugInMenu();
}
