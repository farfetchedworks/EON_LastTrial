#pragma once

#include "mcv_platform.h"
#include "entity/entity.h"
#include "bt_node.h"
#include "fsm/fsm.fwd.h"
#include "navmesh/navmesh.h"

class IBTService;
class IBTTask;
class IBTDecorator;
class CBTBlackboard;

class CBTContext
{

public:
	// Initialise task, decorator and service nodes (caching components, etc). Called after setting the BT (inside setBT())
	void init();

	// Start/stop executing the tree
	void start();
	void stop();
	
	// Reset the context when the behavior tree (including the blackboard) has changed
	void reset();

	void setBT(const CBTree* bt) { btree = bt; }
	void update(float dt);

	bool isEnabled() { return is_enabled; }

	// store/retrieve the entity that owns this context
	void setOwnerEntity(CHandle h_new_owner) { h_owner = h_new_owner; }
	CHandle getOwnerEntity() const { CEntity* e = h_owner; return e; }

	// define if the context is enabled or not
	void setEnabled(bool new_status) { is_enabled = new_status; }

	void setIsDying(const bool val) { is_dying = val; }
	void setHasEonTargeted(const bool val) { has_eon_targeted = val; }
	
	// get/set the current node or current secondary node
	CBTNode* getCurrentNode(bool in_secondary_tree = false);
	void setCurrentNode(CBTNode* new_current_node, bool in_secondary_tree = false);

	bool canBeCancelledOnHit() const;
	bool canBeCancelledOnDecision() const;
	bool isDying() const;
	bool getHasEonTargeted() const { return has_eon_targeted; }

	// called by a node when it has finished
	void onChildFinished(CBTNode* child_node, float dt, EBTNodeResult child_result);

	// called by composite nodes and decorators to check the result
	EBTNodeResult getCurrentResult(bool in_secondary_tree = false);

	// Called by a composite node to get the next child to execute
	CBTNode* getNextNode(bool in_secondary_tree = false);

	void renderInMenu();

	const CBTree* getTree() const { return btree; }

	void setActiveAnimVariable(const std::string& var) { active_anim_variable = var; }

	// Returns a handle to the requested component type, owned by the BT's owner entity
	template< typename TComp >
	CHandle getComponent() const {
		CEntity* e = h_owner;
		if (!e) {
			return CHandle();
		}
		return e->get<TComp>();
	}

	// Attach/detach a decorator as an observer of a specific key
	bool attachObserver(CBTNode* observer_deco_node, EBTNodeResult current_result);
	bool detachObserver(CBTNode* observer_deco_node);

	// Attach/detach a background decorator
	void addBGDecorator(IBTDecorator* bg_deco);
	void removeBGDecorator(IBTDecorator* bg_deco);

	// Add service to the services vector
	void addService(CBTNode* owner_node, IBTService* new_service);


	// Notify all observers when a key has been modified
	void notifyObservers();

	// Get/set blackboard
	void setBlackboard(CBTBlackboard* new_blackboard) { blackboard = new_blackboard; }
	CBTBlackboard* getBlackboard() { return blackboard; }

	// Set variable of the animation FSM associated to the AI owner entity
	void setFSMVariable(const std::string& name, fsm::TVariableValue value);
	fsm::TVariableValue getFSMVariable(const std::string& name);

	// Resets the result of an ABORT BOTH node to FAILED for cases when the node has exited and we still want it to abort
	void resetAbortsBothNode(CBTNode* observer_deco_node);

	// Persists the values stored by tasks of a specific context					
	using TNodeVariable = std::variant<int, float, VEC3, TNavPath, bool, std::function<void(CBTContext& ctx)>, std::string>;
	std::unordered_map<std::string_view, std::unordered_map<std::string, TNodeVariable>> task_states;

	// Sets the value of the entry with the specified name with the type defined by the variant
	void setNodeVariable(const std::string_view& node_name, const std::string& var_name, TNodeVariable value)
	{
		task_states[node_name][var_name] = value;
	}

	// Gets the value of the entry with the specified name
	template <typename T>
	T getNodeVariable(const std::string_view& node_name, const std::string& var_name) 
	{
		return std::get<T>(task_states[node_name][var_name]);
	}

	bool isNodeVariableValid(const std::string_view& node_name, const std::string& var_name)
	{ 
		auto it_node = task_states.find(node_name);
		if (it_node != task_states.end()) {
			auto it_states = task_states[node_name].find(var_name);
			return it_states != task_states[node_name].end();
		}
		return false;
	}

	void allowAborts(bool is_enabled) { can_abort = is_enabled; }

private:
	const CBTree*	btree = nullptr;																			// behavior tree of the context
	CHandle	h_owner;																										// owner of the behavior tree
	CBTNode* current_node = nullptr;																		// current node being executed
	CBTNode* current_secondary_node = nullptr;													// current secondary node being executed
	EBTNodeResult	current_result = EBTNodeResult::SUCCEEDED;
	EBTNodeResult	current_secondary_result = EBTNodeResult::SUCCEEDED;	// current result of the secondary tree of a parallel node
	CBTBlackboard* blackboard = nullptr;																// blackboard of the context (copy of a BB obtained as a resource)
	EBTObserverAborts current_abort_type = EBTObserverAborts::NONE;			// store the last observer abort type so that the parent can clean or not its services and decorators

	std::string active_anim_variable = "";															// stores the animation variable that is currently active in the context
	void stopAnimations();

	bool is_enabled = true;

	bool can_abort = true;																							// set to false when an animation is being executed and the tree cannot abort

	bool is_dying = false;	// Checked when enemy enters a death state, used to avoid camera from locking onto dead enemies
	bool has_eon_targeted = false; // Checked for music interaction purposes, set when Eon is detected (service)

	// Removes all the decorators in the observers vector that are children of the node
	void cleanUpNodeObservers(CBTNode* node);
	
	// Removes all the services in the active services vector that are children of the node
	void cleanUpNodeServices(CBTNode* node);

	/*
	 Vector with the nodes with decorators (observers)

	 Note 1: First it was though as a map:
	 std::map< std::string, std::vector<CBTNode> >
		In this manner, each decorator is added as an observer of a specific blackboard key, making it more efficient to notify
	 only the decorators associated to the changed key
		However, how can we know if the decorator ONLY depends on that key and not any more inside its code?
		Maybe we just want to know which are the decorators interested to observe changes in the blackboard.
	 When there is a change in the blackboard, all observers will be notified

	 Note 2: this vector is treated as a queue: the decorator with the highest priority will be placed first, and all subsequent decorators
	 will be pushed back. The decorator that aborts will handle the situation depending on the ObserverAborts type
*/
	struct SBTObservers {
		CBTNode* decorator_node = nullptr;
		EBTNodeResult previous_result = EBTNodeResult::SUCCEEDED;
	};

	std::vector<SBTObservers> observer_decorators;


	/*
		Decorators which are executed in the background like services. 
		They are checked with the same OnCheckDecoratorAborts but not treated like other decorators
		They are added by the decorators themselves
		They are removed when they finish their execution by the decorators themselves
	*/
	std::vector<IBTDecorator*> background_decorators;

	// Struct and vector of services with their owner node and their actual service
	struct SBTServices {
		CBTNode* owner_node = nullptr;
		IBTService* service = nullptr;
	};

	std::vector<SBTServices> active_services;
};
