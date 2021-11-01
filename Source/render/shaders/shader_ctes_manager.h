#pragma once

class CBaseShaderCte;

class CShaderCtesManager {
public:

	using TFactory = std::function< CBaseShaderCte* (const char* name, const json& j) >;

	// Returns a new initialized instance of the cte previously registered 
	CBaseShaderCte* createCte(const std::string& name, const json& j)
	{
		auto it = generators.find(name);
		if (it != generators.end())
			return it->second(name.c_str(), j);

		// Not found in the map, return null, can't create or the type is unknown...
		return nullptr;
	}

	// A template function that will return a TShaderCteFactory function
	// which creates a new instances, loads from json, updates the gpu
	template< typename T >
	static CShaderCtesManager::TFactory makeFactory(int slot = CB_SLOT_CUSTOM)
	{
		return [=](const char* name, const json& j) -> CBaseShaderCte* {
			CShaderCte< T >* cte = new CShaderCte< T >();
			bool is_ok = cte->create(slot, name);
			assert(is_ok);
			from_json(j, *cte);
			cte->updateFromCPU();
			return cte;
		};
	}	
	
	// A template function that will return a TShaderCteFactory function
	// which creates a new instances, does not load or updates ctes to the gpu
	// but at least the size is automatic
	template< typename T >
	static CShaderCtesManager::TFactory makeBasicFactory(int slot = CB_SLOT_CUSTOM)
	{
		return [=](const char* name, const json& j) -> CBaseShaderCte* {
			CShaderCte< T >* cte = new CShaderCte< T >();
			bool is_ok = cte->create(slot, name);
			assert(is_ok);
			// No reader
			return cte;
		};
	}

	void registerStruct(const char* name, TFactory factory)
	{
		generators[name] = factory;
	}

	bool isDefined(const char* name) const
	{
		return generators.find(name) != generators.end();
	}

private:
	std::unordered_map< std::string, TFactory > generators;
};

extern CShaderCtesManager ShaderCtesManager;

