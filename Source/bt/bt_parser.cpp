#include "mcv_platform.h"
#include "bt_parser.h"

#include "bt.h"
#include "bt_blackboard.h"

void CBTParser::registerTypes()
{
	task_types[""] = new CBTTaskFactory<CBTTaskDummy>();
	decorator_types[""] = new CBTDecoratorFactory<CBTDecoratorDummy>();
	service_types[""] = new CBTServiceFactory<CBTServiceDummy>();
}

IBTTask* CBTParser::createTask(const std::string& type)
{
	auto it = task_types.find(type);
	if (it == task_types.cend())
	{
		fatal("Undefined BT task type %s", type.c_str());
		return nullptr;
	}

	IBTTask* new_task = it->second->create();
	new_task->type = it->first;
	return new_task;
}

IBTDecorator* CBTParser::createDecorator(const std::string& type)
{
	auto it = decorator_types.find(type);
	if (it == decorator_types.cend())
	{
		fatal("Undefined decorator type %s", type.c_str());
		return nullptr;
	}

	IBTDecorator* new_decorator = it->second->create();
	new_decorator->type = it->first;
	return new_decorator;
}

IBTService* CBTParser::createService(const std::string& type)
{
	auto it = service_types.find(type);
	if (it == service_types.cend())
	{
		fatal("Undefined service type %s", type.c_str());
		return nullptr;
	}

	IBTService* new_service = it->second->create();
	new_service->type = it->first;
	return new_service;
}

void parseProbabilityWeights(const json& j, CBTNode* random_node) {
	assert(j.contains("properties"));

	// Load weights
	json weights_j = j["properties"][0];
	std::vector<int> weights = loadVECN(weights_j, "value");

	assert(weights.size() == j["childNodes"].size());

	random_node->setProbabilityWeights(weights);
}

bool CBTParser::parse(CBTree* btree, CBTNode* parent, const json& j)
{
	// Assert current node has a type
	std::string type_str = j.value("type", "");
	assert(type_str != "");

	std::string node_name;
	static unsigned int autoID = 0;

	// If custom label exists, thats the nodes name
	if (j.contains("label")) {
		node_name = j.value("label", "NONE");
		node_name += "_" + std::to_string(autoID);
	}
	else {
		node_name = type_str + std::to_string(autoID);
	}
	++autoID;

	// If parent is null create root node
	if (parent == nullptr) {
		
		// If the root has only one decorator which is of type "Blackboard", load the variables in the "properties" array
		if (j.contains("decorators")) {
			if (j["decorators"].size() == 1 && j["decorators"][0].value("type", "") == "Blackboard")
				btree->blackboard->load(j["decorators"][0]["properties"]);
		}

		// Create root node
		btree->createRootNode(node_name, CBTParser::stringToNodeType(type_str));

		if (j.contains("childNodes")) {
			// Recursion
			for (auto& child : j["childNodes"]) {
				parse(btree, btree->root, child);
			}

			// If the type is Random, read and set probability weights
			if (CBTParser::stringToNodeType(type_str) == EBTNodeType::RANDOM) {
				parseProbabilityWeights(j, btree->root);
			}
		}
	}
	else {
		CBTNode* tail = parent;

		// Check if there are any decorators
		if (j.contains("decorators")) {
			// Add decorator nodes as a chain of nodes

			int deco_num = 1;											// to support one node to have more than one decorator of the same type

			for (auto& dec : j["decorators"]) {
				std::string decorator_type = dec.value("type", "");

				IBTDecorator* new_decorator = CBTParser::createDecorator(decorator_type);

				// ToDo: delegate decorator parsing to the decorator class (the same should be done for services and tasks as each one has their own properties)
				if (dec.contains("properties")) {
					// Add properties to decorator
					for (auto& prop : dec["properties"]) {
						if (prop["name"] == "Number_Field") {
							new_decorator->number_field = prop["value"];
						}
						else if (prop["name"] == "Number_Field_2") {
							new_decorator->number_field_2 = prop["value"];
						}
						else if (prop["name"] == "String_Field") {
							new_decorator->string_field = prop["value"];
						}
						else if (prop["name"] == "String_Field_2") {
							new_decorator->string_field_2 = prop["value"];
						}
						else if (prop["name"] == "ObserverAborts") {
							if (prop["value"] == "None") new_decorator->obs_abort_type = EBTObserverAborts::NONE;
							else if (prop["value"] == "Self") new_decorator->obs_abort_type = EBTObserverAborts::SELF;
							else if (prop["value"] == "Lower Priority") new_decorator->obs_abort_type = EBTObserverAborts::LOWER_PRIORITY;
							else if (prop["value"] == "Both") new_decorator->obs_abort_type = EBTObserverAborts::BOTH;
						}
					}
				}

				// save the inverse check condition into the decorator
				new_decorator->inverse_check_condition = dec.value("inverseCheckCondition", false);
				new_decorator->check_periodically = dec.value("periodic", false);

				std::string deco_name = "deco_" + node_name + "_" + decorator_type + "_" + std::to_string(deco_num);
				new_decorator->name = deco_name;
				tail = btree->addDecoratorNode(tail->getName(), deco_name, new_decorator);
				deco_num++;
			}
		}


		CBTNode* new_node = nullptr;

		// Check if the node is composite
		if (j.contains("childNodes")) {
			// Add composite node to node tail

			new_node = btree->addCompositeNode(tail->getName(), node_name, CBTParser::stringToNodeType(type_str));

			// If the type is Random, read and set probability weights
			if (CBTParser::stringToNodeType(type_str) == EBTNodeType::RANDOM) {
				parseProbabilityWeights(j, new_node);
			}

			// Recursion
			for (auto& child : j["childNodes"]) {
				parse(btree, new_node, child);
			}
		}
		else {
			// Add task node to node tail
			std::string task_type = j.value("type", "");

			IBTTask* new_task = CBTParser::createTask(task_type);
			new_task->name = node_name;

			if (j.contains("properties")) {
				// Add properties to decorator
				for (auto& prop : j["properties"]) {
					if (prop["name"] == "Cancellable on hit") {
						new_task->cancellable_on_hit = prop["value"];
					}
					else if (prop["name"] == "Cancellable on decision") {
						new_task->cancellable_on_decision = prop["value"];
					}
					else if (prop["name"] == "Number_Field_0") {
						new_task->number_field[0] = prop["value"];
					}
					else if (prop["name"] == "Number_Field_1") {
						new_task->number_field[1] = prop["value"];
					}
					else if (prop["name"] == "Number_Field_2") {
						new_task->number_field[2] = prop["value"];
					}
					else if (prop["name"] == "Number_Field_3") {
						new_task->number_field[3] = prop["value"];
					}
					else if (prop["name"] == "String_Field") {
						new_task->string_field = prop["value"];
					}
					else if (prop["name"] == "Bool_Field") {
						new_task->bool_field = prop["value"];
					}
				}
			}

			new_node = btree->addTaskNode(tail->getName(), node_name, new_task);
		}

		// Check if there are any services
		if (j.contains("services")) {
			// Add service nodes as a chain of nodes

			for (auto& service : j["services"]) {
				std::string service_type = service.value("type", "");

				IBTService* new_service = CBTParser::createService(service_type);

				if (service.contains("properties")) {
					// Add properties to decorator
					for (auto& prop : service["properties"]) {
						if (prop["name"] == "Number_Field") {
							new_service->number_field = prop["value"];
						}
						else if (prop["name"] == "String_Field") {
							new_service->string_field = prop["value"];
						}
					}
				}

				new_service->period = service.value("timeBetweenTicks", 0.5f);

				std::string service_name = "serv_" + node_name + "_" + service_type;
				new_service->name = service_name;
				new_node->addService(new_service);
			}
		}

		assert(new_node);
	}

	return true;
}

EBTNodeType CBTParser::stringToNodeType(const std::string& str)
{
	EBTNodeType res;
	if (str == "Sequence") {
		res = EBTNodeType::SEQUENCE;
	}
	else if (str == "Selector") {
		res = EBTNodeType::SELECTOR;
	}
	else if (str == "Random") {
		res = EBTNodeType::RANDOM;
	}
	else if (str == "Parallel") {
		res = EBTNodeType::PARALLEL;
	}
	else if (str == "Task") {
		res = EBTNodeType::TASK;
	}
	else {
		fatal("Unsupported node type at behaviour tree\n");
	}

	return res;
}