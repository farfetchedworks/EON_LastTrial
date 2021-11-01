#pragma once

#include "bt_common.h"
#include "bt_task.h"
#include "bt_decorator.h"
#include "bt_service.h"

class CBTNode;
class CBTree;
class CBTContext;

class CBTNode {

	friend class CBTree;
	friend class CBTContext;
	friend class CBTParser;

	EBTNodeType								type;
	std::string								name;
	std::vector<CBTNode*>			children;
	CBTNode* parent;
	CBTNode* right;
	CBTNode* current_child = nullptr;
	CBTNode* current_child_parallel = nullptr;
	IBTTask* node_task = nullptr;
	IBTDecorator* node_decorator = nullptr;
	IBTService* node_service = nullptr;
	std::vector<int>					probability_weights;
	std::vector<IBTService*>	services;

	bool										is_secondary_node = false;				// Defines if the node is in the secondary subtree of a parallel node

	CBTNode* chooseNodeRandomly();

public:

	EBTNodeResult status = EBTNodeResult::SUCCEEDED;

	EBTNodeType getType() const { return type; }
	IBTTask* getNodeTask() const { return node_task; }

	CBTNode(std::string name);
	bool isRoot();
	void setParent(CBTNode* node);
	void setRight(CBTNode* node);

	// Returns the number of children after the insertion
	size_t addChild(CBTNode* node, EBTNodeType node_type);

	void setType(EBTNodeType new_node_type);
	void setProbabilityWeights(std::vector<int>& weights);

	// TODO: Create subclasses to avoid this mess...

	// Called when the node is entered during execution 
	void onUpdate(CBTContext& context, float dt);
	void onUpdateTask(CBTContext& context, float dt);
	void onUpdateDecorator(CBTContext& context, float dt);
	void onUpdateSequence(CBTContext& context, float dt);
	void onUpdateSelector(CBTContext& context, float dt);
	void onUpdateRandom(CBTContext& context, float dt);
	void onUpdateParallel(CBTContext& context, float dt);


	// Called when the node is entered the first time
	void onEnter(CBTContext& context);
	void onEnterTask(CBTContext& context);
	void onEnterDecorator(CBTContext& context);
	void onEnterSequence(CBTContext& context);
	void onEnterSelector(CBTContext& context);
	void onEnterRandom(CBTContext& context);
	void onEnterParallel(CBTContext& context);

	// Called when a child has aborted
	void onChildAborted(CBTContext& context, float dt, EBTObserverAborts abort_type);

	/*
		If the decorator node has been added as an observer of the blackboard, this method will be called
		whenever a value has changed so that the decorator is executed again.
		The previous result is sent by the context so that the decorator can decide if it has to abort
	*/
	bool onCheckDecoratorAborts(CBTContext& context, float dt, EBTNodeResult previous_result);

	// adds/removes itself as an observer of the blackboard
	void addDecoratorAsObserver(CBTContext& context, EBTNodeResult current_result);
	void removeDecoratorAsObserver(CBTContext& context);

	/*
		If the child is a task, set it as current child and let the context execute it on the next frame
		If the child is a composite or a decorator, call its onUpdate method to execute it on the same frame
	*/
	void updateChild(CBTContext& context, float dt);

	// Sets the current child of a main task or a parallel task if the tree is executing a parallel node
	void setCurrentChild(CBTNode* new_child_node);

	const std::string& getName();

	void renderInMenu() const;

	bool isTypeComposite() const;

	// clear the node and its children
	~CBTNode() {
		for (auto& child : children) {
			delete child;
		}
	}

	// Add service to the current node
	void addService(IBTService* new_service) {
		services.push_back(new_service);
	}
};