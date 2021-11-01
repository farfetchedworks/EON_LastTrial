#include "mcv_platform.h"
#include "bt_blackboard.h"
#include "bt_decorator.h"
#include "bt_node.h"
#include "bt_context.h"

#include "entity/entity.h"

void CBTBlackboard::load(const json& jData)
{
	for (auto& j : jData) {
		std::string type_name = j.value("name", "");
		// parse the value to obtain the keys of the type
		std::stringstream value_string(j.value("value", ""));

		// keys are separated with commas. This obtains each one and creates a key with the type for each one
		while (value_string.good()) {
			std::string substr;												// substring to separate the string with the comma token
			SBBKeyEntry key_entry;										// key entry to be added to the map
			getline(value_string, substr, ',');				// obtain the key from the string and store it in substr
			initializeKeyValue(type_name, key_entry);	// initialize the value so as to specify the type
			bb_keys[substr] = key_entry;							// add the new entry with the name indicated in the json file
		}
	}
}

void CBTBlackboard::initializeKeyValue(const std::string& type_name, SBBKeyEntry& key_entry)
{
	if (type_name == "int")
	{
		key_entry.key_value = 0;
	}
	else if (type_name == "bool")
	{
		key_entry.key_value = false;
	}
	else if (type_name == "float")
	{
		key_entry.key_value = 0.f;
	}
	else if (type_name == "string")
	{
		key_entry.key_value = "";
	}
	else if (type_name == "handle")
	{
		key_entry.key_value = CHandle();
	}
	else if (type_name == "vector")
	{
		key_entry.key_value = VEC3::Zero;
	}
	else if (type_name == "quaternion")
	{
		key_entry.key_value = QUAT::Identity;
	}
}


bool CBTBlackboard::clearValue(const std::string& key_name)
{
	auto it = bb_keys.find(key_name);
	if (it != bb_keys.cend()) {
		bb_keys[key_name].is_set = false;
		bb_keys[key_name].key_value = TBTBlackboardKey();
		return true;
	}
	return false;
}

void CBTBlackboard::renderInMenu()
{

}