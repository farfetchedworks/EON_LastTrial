#pragma once
#include "bt_node.h"
#include "bt_blackboard.h"
#include "resources/resources.h"

// Implementation of the behavior tree
// uses the BTnode so both work as a system
// tree implemented as a map of btnodes for easy traversal
// behaviours are a map to member function pointers, to 
// be defined on a derived class. 
// BT is thus designed as a pure abstract class, so no 
// instances or modifications to bt / btnode are needed...

class CBTree : public IResource 
{
	friend class CBTParser;

private:
	std::map<std::string, CBTNode*> tree;

	CBTBlackboard* blackboard = nullptr;

	CBTNode* root = nullptr;

	CBTNode* createNode(std::string name);
	CBTNode* findNode(std::string name);

	// use these two calls to declare the root and the children. 
	CBTNode* createRootNode(std::string name, EBTNodeType node_type);

	// Add nodes to the tree
	CBTNode* addCompositeNode(std::string parent_name, std::string node_name, EBTNodeType node_type);
	CBTNode* addTaskNode(std::string parent_name, std::string node_name, IBTTask* bt_task = nullptr);
	CBTNode* addDecoratorNode(std::string parent_name, std::string node_name, IBTDecorator* bt_decorator = nullptr);
	CBTNode* addServiceNode(std::string parent_name, std::string node_name, IBTService* bt_service = nullptr);

	// Clear the BT when the resource has changed
	void clear();

public:
	CBTree() : blackboard(new CBTBlackboard()) {}
	void init() const;
	
	CBTNode* current = nullptr;
	CBTNode* current_secondary = nullptr;

	void setCurrent(CBTNode* node);

	CBTBlackboard* getBlackboard() const { return blackboard; }
	CBTNode* getRoot() const { return root; }
	
	// For hotreload for behavior trees
	void onFileChange(const std::string& strfilename) override;

	bool renderInMenu() const;

};
