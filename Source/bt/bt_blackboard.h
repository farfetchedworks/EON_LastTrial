#pragma once

#include "bt_common.h"
#include "bt_context.h"

class IBTDecorator;
class CBTNode;

class CBTBlackboard
{
public:
	
	// Copy constructor to create a blackboard from an already created resource
	CBTBlackboard(const CBTBlackboard& blackboard) {
		bb_keys = blackboard.bb_keys;
	}

	CBTBlackboard() {}
	
	void load(const json& jData);

	// Sets the value of the entry with the specified name with the type defined by the variant
	template<typename T>
	bool setValue(const std::string& key_name, T value);

	// If the value with the name is found, assign an invalid variant and set its bool "is_set" as false
	bool clearValue(const std::string& key_name);

	// Gets the value of the entry with the specified name
	template <typename T>
	T getValue(const std::string& key_name);

	// Checks if the value is correctly set
	bool isValid(const std::string& key_name) { return bb_keys[key_name].is_set; }

	bool hasKey(const std::string& key_name) { return bb_keys.find(key_name) != bb_keys.end(); }

	void setContext(CBTContext* context) { bt_context = context; }

	void renderInMenu();

private:

	// Map with the blackboard keys with their name and their value
	MBTBlackboardKeys bb_keys;

	CBTContext* bt_context = nullptr;

	void initializeKeyValue(const std::string& type_name, SBBKeyEntry& key_entry);
};

template<typename T>
bool CBTBlackboard::setValue(const std::string& key_name, T value)
{
	auto it = bb_keys.find(key_name);
	if (it != bb_keys.cend()) {
		if (std::holds_alternative<T>(bb_keys[key_name].key_value)) {
			bb_keys[key_name].key_value = value;
			bb_keys[key_name].is_set = true;
			if (bt_context) bt_context->notifyObservers();	// Ask the context to notify the observers when a key has changed
			return true;
		}
	}
	return false;
}

template<typename T>
T CBTBlackboard::getValue(const std::string& key_name)
{
	auto it = bb_keys.find(key_name);
	if (it != bb_keys.cend()) {
		if (bb_keys[key_name].is_set)	return std::get<T>(bb_keys[key_name].key_value);
	}
	return T();
}