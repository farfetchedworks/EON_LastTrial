#include "mcv_platform.h"
#include "bt.h"
#include "bt_decorator.h"
#include "bt_task.h"
#include "bt_parser.h"

#include "components/ai/comp_bt.h"
#include "handle/obj_manager.h"

#pragma region BTResource

//-----------------------------------------------------
// Resource Creation
//-----------------------------------------------------

class CBTResourceType : public CResourceType {
public:
	const char* getExtension(int idx) const { return ".bt"; }
	const char* getName() const { return "BT"; }
	IResource* create(const std::string& name) const
	{
		json jData = loadJson(name);
		if (jData.empty())
		{
			return nullptr;
		}

		CBTree* bt = new CBTree();
		bool ok = CBTParser::parse(bt, nullptr, jData);

		// Uncomment this to use the parseNode method include in this same .cpp
		//bt->parseNode(jData);

		// If there was an error delete the bt
		if (!ok)
		{
			delete bt;
			return nullptr;
		}

		return bt;
	}
};

template<>
CResourceType* getClassResourceType<CBTree>() {
	static CBTResourceType factory;
	return &factory;
}
// -----------------------------------------

#pragma endregion

//------------------------------------------
// Behavior Tree Implementation
//------------------------------------------

CBTNode* CBTree::createNode(std::string name)
{
	if (findNode(name))
	{
		dbg("Error: node %s already exists\n", name.c_str());
		return nullptr;	// error: node already exists
	}

	CBTNode* node = new CBTNode(name);
	tree[name] = node;
	return node;
}

CBTNode* CBTree::findNode(std::string name)
{
	if (tree.find(name) == tree.end()) 
		return nullptr;
	else 
		return tree[name];
}

void CBTree::init() const
{
	// Initialise tasks, decorators and services (caching components, etc)
	std::map<std::string, CBTNode*>::const_iterator it;
	for (it = tree.begin(); it != tree.end(); ++it) {
		CBTNode* node = it->second;
		if (node->getType() == EBTNodeType::TASK)
			node->node_task->init();
		else if (node->getType() == EBTNodeType::DECORATOR)
			node->node_decorator->init();

		// Initialise services
		if (!node->services.empty()) {
			for (auto& service : node->services) {
				service->init();
			}
		}
	}
}

CBTNode* CBTree::createRootNode(std::string name, EBTNodeType type)
{
	assert(root == nullptr);

	CBTNode* node = createNode(name);
	node->setParent(nullptr);
	node->setType(type);
	root = node;

	current = nullptr;
	return node;
}

CBTNode* CBTree::addCompositeNode(std::string parent_name, std::string node_name, EBTNodeType node_type)
{
	CBTNode* parent_node = findNode(parent_name);

	// Assert parent is not task or secondary task
	assert(parent_node->getType() != EBTNodeType::TASK);

	// Assert if parent is parallel, there are no composite children and nchildren < 2
	if (parent_node->getType() == EBTNodeType::PARALLEL) {
		assert(parent_node->children.size() < 2);

		for (auto& child : parent_node->children) {
			assert(!child->isTypeComposite());
		}
	}

	CBTNode* composite_node = createNode(node_name);
	parent_node->addChild(composite_node, node_type);

	return composite_node;
}

CBTNode* CBTree::addTaskNode(std::string parent_name, std::string node_name, IBTTask* bt_task)
{
	CBTNode* parent_node = findNode(parent_name);

	// Assert parent is not task or secondary task
	assert(parent_node->getType() != EBTNodeType::TASK);

	// Assert if parent is parallel, nchildren < 2
	if (parent_node->getType() == EBTNodeType::PARALLEL) {
		assert(parent_node->children.size() < 2);
	}

	CBTNode* task_node = createNode(node_name);
	size_t nchildren = parent_node->addChild(task_node, EBTNodeType::TASK);
	task_node->node_task = bt_task;

	return task_node;
}

CBTNode* CBTree::addDecoratorNode(std::string parent_name, std::string node_name, IBTDecorator* bt_decorator)
{
	CBTNode* parent_node = findNode(parent_name);
	CBTNode* decorator_node = createNode(node_name);
	
	parent_node->addChild(decorator_node, EBTNodeType::DECORATOR);
	decorator_node->node_decorator = bt_decorator;
	
	return decorator_node;
}

CBTNode* CBTree::addServiceNode(std::string parent_name, std::string node_name, IBTService* bt_service)
{
	CBTNode* parent_node = findNode(parent_name);
	CBTNode* service_node = createNode(node_name);

	parent_node->addChild(service_node, EBTNodeType::SERVICE);
	service_node->node_service = bt_service;

	return service_node;
}

void CBTree::setCurrent(CBTNode* node)
{
	current = node;
}


void CBTree::clear()
{
	delete root;
	tree.clear();
	delete blackboard;
	root = nullptr;
}

void CBTree::onFileChange(const std::string& filename)
{
	if (getName() == filename)
	{
		json jData = loadJson(filename);
		if (jData.empty())
		{
			return;
		}

		// Clear BT
		clear();

		// Create the BT again
		blackboard = new CBTBlackboard();
		bool ok = CBTParser::parse(this, nullptr, jData);
		assert(ok);

		// Notify each BT component from each AI
		// only if the tree of the component is this tree
		getObjectManager<TCompBT>()->forEach(
			[&](TCompBT* h_comp_bt) {
				
				const CBTree* comp_tree = h_comp_bt->getContext()->getTree();
				if (this == comp_tree) {
					h_comp_bt->reload();
				}
			}
		);
	}	
}


bool CBTree::renderInMenu() const
{
	// Display tasks

	// Display decorators

	if (root) {
		root->renderInMenu();
	}
	else {
		ImGui::Text("Root: ...");
	}

	return true;
}