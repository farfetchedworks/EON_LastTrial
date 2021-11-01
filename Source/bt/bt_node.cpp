#include "mcv_platform.h"
#include "bt_node.h"
#include "bt.h"
#include "bt_context.h"

CBTNode* CBTNode::chooseNodeRandomly()
{
	// Sum all weights
	int sum_of_weights = 0;

	for (int i : probability_weights) {
		sum_of_weights += i;
	}

	// Pick random number > 0 and < sum
	int rand_n = rand() % sum_of_weights;

	// Iterate through weights substracting them from rand_n, until rand_n < item
	for (int child_idx = 0; child_idx < probability_weights.size(); ++child_idx) {
		if (rand_n < probability_weights[child_idx]) {
			return children[child_idx];
		}

		rand_n -= probability_weights[child_idx];
	}

	return nullptr;
}

CBTNode::CBTNode(std::string name)
{
	this->name = name;
}

bool CBTNode::isRoot()
{
	return (parent == nullptr);
}

void CBTNode::setParent(CBTNode* node)
{
	parent = node;
}

void CBTNode::setRight(CBTNode* node)
{
	right = node;
}

void CBTNode::setType(EBTNodeType new_node_type)
{
	type = new_node_type;
}

void CBTNode::setProbabilityWeights(std::vector<int>& weights)
{
	probability_weights = weights;
}

const std::string& CBTNode::getName()
{
	return name;
}

size_t CBTNode::addChild(CBTNode* node, EBTNodeType node_type)
{
	if (!children.empty())													// if this node already had children, connect the last one to this
		children.back()->setRight(node);							// new one so the new one is to the RIGHT of the last one
	
	children.push_back(node);												// in any case, insert it
	node->right = nullptr;													// as we're adding from the right make sure right points to nullptr

	node->setParent(this);
	node->setType(node_type);
	
	{
		// If the parent of the new node (this node) is a parallel node or is in secondary node, set the new node as a secondary node
	
		// The child is child of the parallel parent if the type of the current node is parallel and already has two children
		// If there is only one child, then this is the main task of the parallel node
		bool is_direct_parallel_child = this->type == EBTNodeType::PARALLEL && children.size() == 2;

		// The child is in a secondary subtree if the parent is not parallel and this is a secondary node
		bool is_in_secondary_subtree = this->is_secondary_node;

		node->is_secondary_node = is_direct_parallel_child || is_in_secondary_subtree;
		//node->is_secondary_node = (this->type == EBTNodeType::PARALLEL || this->is_secondary_node) && children.size() == 2;
	}

	return children.size();
}

void CBTNode::renderInMenu() const
{
	if (ImGui::TreeNode(name.c_str()))
	{
		for (auto& child : children)
		{
			child->renderInMenu();
		}

		ImGui::TreePop();
	}
}

bool CBTNode::isTypeComposite() const
{
	return type == EBTNodeType::SEQUENCE || type == EBTNodeType::SELECTOR ||
		type == EBTNodeType::RANDOM || type == EBTNodeType::PARALLEL;
}

//--------------------------------------------------
// Testing new update method
//--------------------------------------------------

void CBTNode::onUpdate(CBTContext& context, float dt)
{
	//dbg("Updating node %s\n", name.c_str());

	switch (type)
	{
	case EBTNodeType::TASK:
		onUpdateTask(context, dt); break;
	case EBTNodeType::RANDOM:
		onUpdateRandom(context, dt); break;
	case EBTNodeType::SELECTOR:
		onUpdateSelector(context, dt); break;
	case EBTNodeType::SEQUENCE:
		onUpdateSequence(context, dt); break;
	case EBTNodeType::DECORATOR:
		onUpdateDecorator(context, dt); break;
	case EBTNodeType::PARALLEL:
		onUpdateParallel(context, dt); break;
	}
}


void CBTNode::onUpdateTask(CBTContext& context, float dt)
{
	EBTNodeResult result;
	//dbg("Executing task %s\n", name.c_str());

	result = this->node_task->executeTask(context, dt);
	
	if (result != EBTNodeResult::IN_PROGRESS) {
		context.onChildFinished(this, dt, result);
	}
}

void CBTNode::updateChild(CBTContext& context, float dt)
{
	CBTNode* next_child = context.getNextNode(is_secondary_node);
	
	// If the next child is not a task, call its update (or onEnter??)
	if (next_child->getType() != EBTNodeType::TASK) {			
		next_child->onUpdate(context, dt);
	}
	// If the next child is a task set it as current node or current secondary node
	else {
		context.setCurrentNode(next_child, is_secondary_node);
	}
		
}

void CBTNode::setCurrentChild(CBTNode* new_child_node)
{
	// If the node is not secondary node and this node is not a parallel, set the current child
	if (!is_secondary_node && type!=EBTNodeType::PARALLEL) {
		current_child = new_child_node;
	}
	else {
		current_child_parallel = new_child_node;
	}
}

void CBTNode::onUpdateDecorator(CBTContext& context, float dt)
{
	// If the decorator is called onUpdate, then this is not the first time the decorator is being analyzed

	//dbg("Evaluating decorator %s\n", name.c_str());
	
	// If the decorator is of loop type (timer or loop) enter the decorator again
	if (node_decorator->isLoopType()) {
		onEnterDecorator(context);
	}
	else {
		// Otherwise, leave the decorator
		
		// If the decorator aborts Lower Priority and is exiting, add itself as observer (in this case, storing the result is unnecessary)
		if (node_decorator->getObserverAborts() == EBTObserverAborts::LOWER_PRIORITY) {
			addDecoratorAsObserver(context, EBTNodeResult::FAILED);
		}
		else {
			removeDecoratorAsObserver(context);

			/*
				If an ABORT BOTH node is exiting, set its current result to FAILED
				This is because if we are running a subtree to the right of this node, we want to get an ABORT when this node is SUCCESS.
				
				If the ABORT BOTH node was successful but its subtree has finished, it is STILL NECESSARY to check if it aborts. The only way that it will
				abort is if its previous result was failed and when the abort is checked it is successful now
			*/
			if (node_decorator->getObserverAborts() == EBTObserverAborts::BOTH) {
				context.resetAbortsBothNode(this);
			}
		}

		node_decorator->onExit(context);
		EBTNodeResult result = context.getCurrentResult(is_secondary_node);

		// Background nodes always return succeeded when exiting because they are entered and exited freely the first time
		// It will only return FAILED when it is entered again and the cooldown has not expired
		if (node_decorator->isBackground())
			result = EBTNodeResult::SUCCEEDED;

		context.onChildFinished(this, dt, result);
	}

}


void CBTNode::onUpdateSequence(CBTContext& context, float dt)
{
	//dbg("Analyzing sequence %s\n", name.c_str());

	CBTNode* next_node = context.getNextNode(is_secondary_node);
	
	// If the current child is set, then the node has begun executing
	EBTNodeResult current_result = context.getCurrentResult(is_secondary_node);

	// If there is no node to the right of the current child, it means all previous children have succeeded
	// therefore, return the result of the last child
	 if (!next_node){
		context.onChildFinished(this, dt, current_result);
		return;
	}

	// If there is a node to the right, check the result of the current child
	switch (current_result) {
		case EBTNodeResult::FAILED:											// If the child failed, the sequence has failed
			context.onChildFinished(this, dt, EBTNodeResult::FAILED);
			break;
		case EBTNodeResult::SUCCEEDED:									// If the child succeeded, go to the next child (ENTER the next child)
			next_node->onEnter(context);
			break;
	}
}

void CBTNode::onUpdateSelector(CBTContext& context, float dt)
{
	//dbg("Analyzing selector %s\n", name.c_str());

	CBTNode* next_node = context.getNextNode(is_secondary_node);
	
	// If the current child is set, then the node has begun executing
	EBTNodeResult current_result = context.getCurrentResult(is_secondary_node);

	// If there is no node to the right of the current child, it means all previous children have failed
	// therefore, return the result of the last child
	if (!next_node) {
		context.onChildFinished(this, dt, current_result);
		return;
	}

	// If there is a node to the right, check the result of the current child
	switch (current_result) {
	case EBTNodeResult::FAILED:												// If the child failed, go to the next one
		next_node->onEnter(context);
		break;
	case EBTNodeResult::SUCCEEDED:										// If the child succeeded, go back to the current parent
		context.onChildFinished(this, dt, EBTNodeResult::SUCCEEDED);
		return;
	}

}

void CBTNode::onUpdateRandom(CBTContext& context, float dt)
{
	//dbg("Analyzing random node %s\n", name.c_str());
	
	EBTNodeResult current_result = context.getCurrentResult(is_secondary_node);
	context.onChildFinished(this, dt, current_result);
}

void CBTNode::onUpdateParallel(CBTContext& context, float dt)
{
	//dbg("Analyzing parallel node %s\n", name.c_str());
	
	// obtain the current result from the main and the secondary subtrees
	EBTNodeResult current_main_result = context.getCurrentResult(false);
	EBTNodeResult current_secondary_result = context.getCurrentResult(true);

	// If the main parallel node finished
	if (current_main_result != EBTNodeResult::IN_PROGRESS) {
		// remove any secondary node from its current execution. Set the current secondary node to nullptr
		context.setCurrentNode(nullptr, true);

		// return its result (Finish mode: immediate)
		context.onChildFinished(this, dt, current_main_result);	
	}
	// If the main parallel node did not finish, enter again the secondary subtree
	else {
		CBTNode* secondary_child = children[1];
		secondary_child->onEnter(context);
	}
}

bool CBTNode::onCheckDecoratorAborts(CBTContext& context, float dt, EBTNodeResult previous_result)
{
	//dbg("Blackboard changed %s\n", name.c_str());
	EBTNodeResult result = node_decorator->evaluateCondition(context, dt);

	// If the node is looping and has succeeded, return aborted
	if (result == EBTNodeResult::SUCCEEDED && node_decorator->isLoopType()) {
		context.onChildFinished(this, dt, EBTNodeResult::SUCCEEDED);							// Aborting loops must return succeeded (such as a timer) because it is not an exact abort
		return true;
	}

	// If the node is a background node and has succeeded, return aborted
	if (result == EBTNodeResult::SUCCEEDED && node_decorator->isBackground()) {
		context.onChildFinished(this, dt, EBTNodeResult::ABORTED);
		return true;
	}

	switch (node_decorator->getObserverAborts()) {

		/*
			SELF: only abort if failed. If it is successful, then the subtree under the decorator is being ticked
			and the decorator was successful before.
			These nodes are removed as observers when they exit, because they only had to abort themselves and their subtree
		*/
	case EBTObserverAborts::SELF:
		if (result == EBTNodeResult::FAILED) {
			context.onChildFinished(this, dt, EBTNodeResult::ABORTED);
			return true;
		}
		break;

		/*
			LOWER: these nodes will only be checked on abort when they succeed. They are added as observers in these cases:
			- When they are not successful, so they must abort the execution of its right siblings
			- When they were successful, but one of its child chained decorators was not
			- When this node exits
		*/
	case EBTObserverAborts::LOWER_PRIORITY:
		if (result == EBTNodeResult::SUCCEEDED) {
			context.onChildFinished(this, dt, EBTNodeResult::ABORTED);
			return true;
		}
		break;
		
		/*
			for BOTH we must save the previous result (in "onUpdateDecorator") and check if it is different to the current one
			If we are executing a node below this decorator and it aborts => the previous result was success and now it fails, so it must abort
			If we are executing a node that is not in this decorator's subtree => the previous result was failed and now it is successful, so it must abort.
			If we entered the subtree of this node, and now we exit it, we must set its previous result to FAILED in "onUpdateDecorator" so that, if it succeeds
			then it must abort the execution
		*/
	case EBTObserverAborts::BOTH:
		if (previous_result != result) {
			context.onChildFinished(this, dt, EBTNodeResult::ABORTED);
			return true;
		}		 
	}

	return false;
}

void CBTNode::addDecoratorAsObserver(CBTContext& context, EBTNodeResult current_result)
{
	// Only add it as an observer if the observer aborts something
	if (node_decorator->getObserverAborts() != EBTObserverAborts::NONE) {
		context.attachObserver(this, current_result);
	}
}

void CBTNode::removeDecoratorAsObserver(CBTContext& context)
{
	// Only remove itself as an observer if the observer aborts self. Otherwise, the parent must make a decision
	if (node_decorator->getObserverAborts() == EBTObserverAborts::SELF) {
		context.detachObserver(this);
	}
}

void CBTNode::onEnter(CBTContext& context)
{
	//dbg("Entering node: %s\n", name.c_str());

	// Always add the services of the node to the vector of current services
	if (!services.empty()) {
		for (auto service : services) {
			context.addService(this, service);
			service->onEnter(context);
		}
	}
	
	switch (type)
	{
	case EBTNodeType::TASK:
		onEnterTask(context); break;
	case EBTNodeType::RANDOM:
		onEnterRandom(context); break;
	case EBTNodeType::SELECTOR:
		onEnterSelector(context); break;
	case EBTNodeType::SEQUENCE:
		onEnterSequence(context); break;
	case EBTNodeType::DECORATOR:
		onEnterDecorator(context); break;
	case EBTNodeType::PARALLEL:
		onEnterParallel(context); break;
	}
}

// when the task is entered, it only sets itself as the current node or current secondary node
void CBTNode::onEnterTask(CBTContext& context)
{
	node_task->onEnter(context);
	context.setCurrentNode(this, is_secondary_node);
}

void CBTNode::onEnterRandom(CBTContext& context)
{
	CBTNode* child = chooseNodeRandomly();
	context.setCurrentNode(this, is_secondary_node);
	child->onEnter(context);
}

void CBTNode::onEnterSelector(CBTContext& context)
{
	CBTNode* child = children[0];
	context.setCurrentNode(this, is_secondary_node);
	child->onEnter(context);
}

void CBTNode::onEnterSequence(CBTContext& context)
{
	CBTNode* child = children[0];
	context.setCurrentNode(this, is_secondary_node);
	child->onEnter(context);
}

void CBTNode::onEnterDecorator(CBTContext& context)
{
	float dt = Time.delta;
	
	// initialize the decorator
	node_decorator->onEnter(context);

	EBTNodeResult result = node_decorator->evaluateCondition(context, dt);
	EBTObserverAborts obs_abort_type = node_decorator->getObserverAborts();

	context.setCurrentNode(this, is_secondary_node);
	

	// If the decorator failed the first time
	if (result == EBTNodeResult::FAILED) {
		
		// If it failed and the node aborts lower or both, add as observer
		if (obs_abort_type != EBTObserverAborts::SELF) {
			addDecoratorAsObserver(context, result);
		}

		// return failed to the context
		context.onChildFinished(this, dt, EBTNodeResult::FAILED);
	}
	// If the decorator was successful
	else {

		// If the node is loop type and has succeeded, return succeeded to its parent
		if (result == EBTNodeResult::SUCCEEDED && node_decorator->isLoopType()) {
			context.onChildFinished(this, dt, EBTNodeResult::SUCCEEDED);
		}
		else {
			// If the node aborts self or both, add as observer
			if (obs_abort_type != EBTObserverAborts::LOWER_PRIORITY) {
				addDecoratorAsObserver(context, result);
			}
			// enter the only child node
			CBTNode* child = children[0];
			child->onEnter(context);
		}
	}
}

void CBTNode::onEnterParallel(CBTContext& context)
{
	context.setCurrentNode(this, is_secondary_node);			// set this node as the current node
	
	// set the first child as the main node to be executed
	CBTNode* child = children[0];
	child->onEnter(context);
	
	// set the second child as the secondary node to be executed
	CBTNode* secondary_child = children[1];
	secondary_child->onEnter(context);

}

void CBTNode::onChildAborted(CBTContext& context, float dt, EBTObserverAborts abort_type)
{

	switch (type)
	{
	
		// If RANDOM is ABORTED=> SELF: the node fails | BOTH/LOWER: enter the random node again
	case EBTNodeType::RANDOM:
		if (abort_type == EBTObserverAborts::SELF) {
			context.setCurrentNode(this, is_secondary_node);			// set this node as the current node
			context.onChildFinished(this, dt, EBTNodeResult::FAILED);
		}
		else {
			onEnterRandom(context); 
		}
		break;
	
		// SELECTOR ABORTED=> SELF: go to the next node | BOTH/LOWER: enter the node again
	case EBTNodeType::SELECTOR:
		if (abort_type == EBTObserverAborts::SELF) {
			CBTNode* next_node = context.getNextNode(is_secondary_node);

			// If there is no node to the right of the current child, it means all previous children have failed
			// therefore, return fail
			if (!next_node) {
				context.onChildFinished(this, dt, EBTNodeResult::FAILED);
			}
			else {
				next_node->onEnter(context);
			}
		}
		else {
			onEnterSelector(context);
		}
		break;
	
		// SEQUENCE ABORTED=> the node always fails (MAYBE IF IT IS LOWER, ENTER THE SEQUENCE :-?)
	case EBTNodeType::SEQUENCE:
		context.setCurrentNode(this, is_secondary_node);			// set this node as the current node
		context.onChildFinished(this, dt, EBTNodeResult::FAILED);
		break;
	
		// PARALLEL ABORTED=> if main aborted: the node fails | if second aborted: enter the secondary node again (only if SELF or BOTH) 
	case EBTNodeType::PARALLEL:{
			// obtain the current result from the main and the secondary subtrees
			EBTNodeResult current_main_result = context.getCurrentResult(false);
			EBTNodeResult current_secondary_result = context.getCurrentResult(true);

			// If the main parallel node finished
			if (current_main_result == EBTNodeResult::ABORTED) {
				// remove any secondary node from its current execution. Set the current secondary node to nullptr
				context.setCurrentNode(nullptr, true);
				
				// set itself as the current main node and return failed
				context.setCurrentNode(this, is_secondary_node);			
				context.onChildFinished(this, dt, EBTNodeResult::FAILED);
				break;
			}

			if (current_secondary_result == EBTNodeResult::ABORTED && abort_type != EBTObserverAborts::LOWER_PRIORITY) {
				// set the second child as the secondary node to be executed
				CBTNode* secondary_child = children[1];
				secondary_child->onEnter(context);
			}
			break;
		}
	
		// DECORATOR will hand the abort result to its parent until someone handles it
	case EBTNodeType::DECORATOR:
		context.setCurrentNode(this, is_secondary_node);			// set this node as the current node
		context.onChildFinished(this, dt, EBTNodeResult::ABORTED);
		break;
	}
}
