#include "mcv_platform.h"
#include "bt_context.h"
#include "bt.h"
#include "components/messages.h"
#include "components/common/comp_fsm.h"

void CBTContext::update(float dt)
{
	CEntity* owner = getOwnerEntity();
	std::string owner_name = owner->getName();
	PROFILE_FUNCTION("Behavior Tree Context" + owner_name);
	if (!is_enabled)
		return;
	
	{
		PROFILE_FUNCTION("BT: Services");
		// If there are stacked services, run them every frame
		for (auto service : active_services) {
			service.service->tickService(*this, dt);
		}
	}
	
	{
		PROFILE_FUNCTION("BT: Execution");
		// If there is a current node, update it. Otherwise, start from the root
		if (current_node) {
			current_node->onUpdate(*this, dt);
		}
		else {
			btree->getRoot()->onEnter(*this);
		}

		// If there is a secondary node, update it
		if (current_secondary_node) {
			current_secondary_node->onUpdate(*this, dt);
		}
	}


	{
		PROFILE_FUNCTION("BT: Observers");
		if (can_abort) {
			for (auto deco_it = observer_decorators.begin(); deco_it != observer_decorators.cend(); deco_it++) {

				CBTNode* deco_node = (*deco_it).decorator_node;
				if (deco_node->node_decorator->isCheckedPeriodically()) {
					bool has_aborted = deco_node->onCheckDecoratorAborts(*this, dt, (*deco_it).previous_result);

					// if it aborted, leave the loop
					if (has_aborted) {
						break;
					}
				}
			}
		}
	}

	{
		PROFILE_FUNCTION("BT: Background decorators");
		for (auto bg_deco : background_decorators) {
			bg_deco->execBackground(*this, dt);
		}
	}
}

void CBTContext::setCurrentNode(CBTNode* new_current_node, bool in_secondary_tree)
{
	if (!in_secondary_tree) {
		current_node = new_current_node;
	}
	else {
		current_secondary_node = new_current_node;
	}
}

bool CBTContext::canBeCancelledOnHit() const
{
	// current node might be nullptr, in which case, return true
	if (!current_node)
		return true;

	// current node not nullptr, check if it is cancellable on hit
	bool isCancellable = current_node->getNodeTask()->cancellable_on_hit;
	if (!current_secondary_node)
		return isCancellable;

	// theres also a secondary node, both of them should be cancellable to return true
	isCancellable &= current_secondary_node->getNodeTask()->cancellable_on_hit;
	return isCancellable;
}

bool CBTContext::canBeCancelledOnDecision() const
{
	// current node might be nullptr, in which case, return true
	if (!current_node)
		return true;

	// current node not nullptr, check if it is cancellable on decision
	bool isCancellable = current_node->getNodeTask()->cancellable_on_decision;
	if (!current_secondary_node)
		return isCancellable;

	// theres also a secondary node, both of them should be cancellable to return true
	isCancellable &= current_secondary_node->getNodeTask()->cancellable_on_decision;
	return isCancellable;
}

bool CBTContext::isDying() const
{
	return is_dying;
}

CBTNode* CBTContext::getNextNode(bool in_secondary_tree)
{
	if (in_secondary_tree) {
		return current_secondary_node->right;
	}
	else {
		return current_node->right;
	}
}

CBTNode* CBTContext::getCurrentNode(bool in_secondary_tree)
{
	if (in_secondary_tree) {
		return current_secondary_node;
	}
	else {
		return current_node;
	}
}

void CBTContext::onChildFinished(CBTNode* child_node, float dt, EBTNodeResult child_result)
{
	CBTNode* parent_node = child_node->parent;

	// If the child is a composite node, clean up its observer decorators and services
	// The decorators will clean themselves if they are self, or their parents will clean them when the parents finish
	if (child_node->isTypeComposite()) {
		cleanUpNodeObservers(child_node);
		cleanUpNodeServices(child_node);
	}

	// If the child aborted, erase the observers of its subtree, call its parent abort method and return
	if (child_result == EBTNodeResult::ABORTED) {

		// Stop the current animation from the associated FSM
		stopAnimations();

		// If the aborted child is not in a secondary tree, then reset the current secondary node
		if (!child_node->is_secondary_node) {
			setCurrentNode(nullptr, true);
		}

		/*
			 If the current abort type is NONE, set it to the abort type of the node that aborted.
			 This is for cases when there are chained decorators, for example, A->D1->D2->D3->B. If D3 aborted. then we have to store
			 its abort type. When the abort reaches A, this node will know the abort type of the node that aborted (not the abort type of its direct child, D1)
		*/
		if (current_abort_type == EBTObserverAborts::NONE) {
			current_abort_type = child_node->node_decorator->getObserverAborts();
		}

		/*
			If the aborted node is a decorator, clean itself and its children
			If it aborted and it is self, and it has parent decorators with LOWER ot BOTH, they will also be removed
			TO DO: cleanup the services that are in its subtree
		*/
		if (child_node->type == EBTNodeType::DECORATOR) {
			cleanUpNodeObservers(child_node);
			cleanUpNodeServices(child_node);
		}

		// Now the parent node must always know the current abort type, not the abort type of the child that executed last
		parent_node->onChildAborted(*this, dt, current_abort_type);

		// After the parent handled the abort, if it is composite, reset the current abort type
		if (parent_node->isTypeComposite()) {
			current_abort_type = EBTObserverAborts::NONE;
		}

		return;
	}


	// If the child is not in the secondary node, save the result in the current result or go to the root
	if (!child_node->is_secondary_node) {
		if (parent_node) {
			current_result = child_result;
			setCurrentNode(child_node, child_node->is_secondary_node);
			parent_node->onUpdate(*this, dt);
		}
		else {
			setCurrentNode(nullptr);
		}
	}
	// If the child is in the secondary node, save the result in the current secondary result and update the node
	else {
		current_secondary_result = child_result;
		setCurrentNode(child_node, child_node->is_secondary_node);
		parent_node->onUpdate(*this, dt);
	}
}

EBTNodeResult CBTContext::getCurrentResult(bool in_secondary_tree)
{ 
	return (in_secondary_tree ? current_secondary_result : current_result); 
}

bool CBTContext::attachObserver(CBTNode* observer_deco_node, EBTNodeResult current_result)
{
	bool obs_in_vector = false;
	auto deco_it = observer_decorators.begin();
	for (; deco_it != observer_decorators.cend(); deco_it++) {
		if ((*deco_it).decorator_node == observer_deco_node) {
			obs_in_vector = true;
			break;
		}
	}
	
	if (!obs_in_vector) {
			SBTObservers new_observer;
			new_observer.decorator_node = observer_deco_node;
			new_observer.previous_result = current_result;
			observer_decorators.push_back(new_observer);
			return true;
	}
	else {
		// If the decorator was found, save the current result (specially if it is an abort both)
		(*deco_it).previous_result = current_result;
		return false;
	}
	
}

bool CBTContext::detachObserver(CBTNode* observer_deco_node)
{
	// Go through the vector from the beginning (std::vector::erase does not accept reverse iterators)
	// It should be more optimal to do it in reverse because the one to remove is supposed to be the last, unless there are chained decorators
	
	for (auto deco_it = observer_decorators.cbegin(); deco_it != observer_decorators.cend(); deco_it++) {
		
		CBTNode* deco_node = (*deco_it).decorator_node;
		// If the decorator is found
		if (deco_node == observer_deco_node) {
			observer_decorators.erase(deco_it);
			return true;
		}
	}
	return false;
}

void CBTContext::notifyObservers()
{
	// only notify observers which are not checked periodically
	if (!can_abort)
		return;

	for (int i = 0; i < observer_decorators.size(); ++i) {

		SBTObservers& deco = observer_decorators[i];

		if (!deco.decorator_node)
			continue;
		if (!deco.decorator_node->node_decorator->isCheckedPeriodically()) {
			deco.decorator_node->onCheckDecoratorAborts(*this, Time.delta, deco.previous_result);
		}
	}
}

void CBTContext::resetAbortsBothNode(CBTNode* observer_deco_node)
{
	// Search for the decorator node in the observers list starting from the tail
	for (auto deco_it = observer_decorators.rbegin(); deco_it != observer_decorators.rend(); deco_it++) {

		CBTNode* deco_node = (*deco_it).decorator_node;
		// If the decorator is found, set its current result to FAILED
		if (deco_node == observer_deco_node) {
			(*deco_it).previous_result = EBTNodeResult::FAILED;
			return;
		}
	}
}


void CBTContext::setFSMVariable(const std::string& name, fsm::TVariableValue value)
{
	TMsgFSMVariable msg;
	msg.name = name;
	msg.value = value;
	h_owner.sendMsg(msg);
}

fsm::TVariableValue CBTContext::getFSMVariable(const std::string& name)
{
	TCompFSM* c_fsm = getComponent<TCompFSM>();
	return c_fsm->getCtx().getVariable(name)->getValue();
}

void CBTContext::init()
{
	btree->init();
}

void CBTContext::start()
{
	if (!is_enabled)
		return;
	// start from the root
	btree->getRoot()->onEnter(*this);
}

/*
	Clean-up starts from the beginning of the observers vector
	When it finds a decorator which is child of the node, it finishes the loop and clears the vector from that position until the end
	This is because after the current node's decorators, there will not be decorators which are children of other nodes because they have been already cleaned.

	If it finds a decorator that is equal to the current node, then it must clean:
	SELF: itself and its children
	BOTH/LOWER: itself, its children and the decorators in any subtree to the right of the aborted node

	NOTE: if the node received an ABORT, when it finds its first observer child, it deletes the rest of the observers of its subtree because they were
	pushed back to the vector, behind its first observer child
*/
void CBTContext::cleanUpNodeObservers(CBTNode* node)
{
	bool child_was_found = false;
	auto deco_it = observer_decorators.begin();

	for (; deco_it != observer_decorators.cend(); deco_it++) {
		
		CBTNode* current_node = (*deco_it).decorator_node;
		if (current_node->parent == node || current_node == node) {
			child_was_found = true;
			break;
		}
	}

	// If a child was found, erase the vector from the current position
	if (child_was_found) {
		observer_decorators.erase(deco_it, observer_decorators.end());
	}
}

void CBTContext::cleanUpNodeServices(CBTNode* node)
{
	bool service_was_found = false;
	auto service_it = active_services.begin();

	for (; service_it != active_services.cend(); service_it++) {

		CBTNode* service_owner_node = (*service_it).owner_node;
		if (service_owner_node == node) {
			service_was_found = true;
			break;
		}
	}

	// If a child was found, erase the vector from the current position
	if (service_was_found) {
		active_services.erase(service_it, active_services.end());
	}
}

void CBTContext::addService(CBTNode* owner_node, IBTService* new_service)
{
	SBTServices new_active_service;
	new_active_service.owner_node = owner_node;
	new_active_service.service = new_service;

	active_services.push_back(new_active_service);
}

void CBTContext::addBGDecorator(IBTDecorator* bg_deco)
{
	background_decorators.push_back(bg_deco);
}

void CBTContext::removeBGDecorator(IBTDecorator* bg_deco)
{
	auto it_bg_decos = std::find(background_decorators.begin(), background_decorators.end(), bg_deco);
	
	if (it_bg_decos != background_decorators.end()) {
		background_decorators.erase(it_bg_decos);
	}
}

void CBTContext::stopAnimations()
{
	if (active_anim_variable != "") {
		setFSMVariable(active_anim_variable, 0);
		setFSMVariable("combo_attack_active", false);			// Because all combos have stopped and the new animations have to be stopped on exit
	}
}

void CBTContext::stop()
{
	current_node = nullptr;
	current_secondary_node = nullptr;
	current_result = EBTNodeResult::SUCCEEDED;
	current_secondary_result = EBTNodeResult::SUCCEEDED;
	current_abort_type = EBTObserverAborts::NONE;
	is_enabled = false;
}

void CBTContext::reset()
{
	stop();																																																																						
	is_enabled = true;
	//delete blackboard;
	observer_decorators.clear();
	active_services.clear();
	task_states.clear();
	background_decorators.clear();
}

void CBTContext::renderInMenu() 
{
	ImGui::Checkbox("Enabled", &is_enabled);

	if (current_node)
	{
		ImGui::Text("Current Node: %s", current_node->name.c_str());
	}
	else
	{
		ImGui::Text("Current Node: NONE");
	}

	if (current_secondary_node)
	{
		ImGui::Text("Current 2nd Node: %s", current_secondary_node->name.c_str());
	}
	else
	{
		ImGui::Text("Current 2nd Node: NONE");
	}

	ImGui::Separator();

	if (ImGui::TreeNode("Active Services"))
	{
		for (auto& ser : active_services)
		{
			ImGui::Text(ser.service->name.c_str());
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Observer Decorators"))
	{
		for (auto& dec : observer_decorators)
		{
			ImGui::Text(dec.decorator_node->name.c_str());
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Background Decorators"))
	{
		for (auto& dec : background_decorators)
		{
			ImGui::Text(dec->name.c_str());
		}

		ImGui::TreePop();
	}

	ImGui::Separator();

	ImGui::Checkbox("Can Abort", &can_abort);
	ImGui::Checkbox("Is Dying", &is_dying);
}