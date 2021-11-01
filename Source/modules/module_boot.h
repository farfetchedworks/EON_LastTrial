#pragma once

#include "modules/module.h"
#include "entity/entity_parser.h"

class CModuleBoot : public IModule
{
	bool _preloadingResources = false;
	bool previewEnabled = false;
	int total_entries = 1;

	json jBoot;
	int _lastLoaded = 0;
	int _loaded = 0;

	std::vector< TEntityParseContext > ctxs;

public:

	CModuleBoot(const std::string& name) : IModule(name) { }

	bool loadBoot(const std::string& strfilename);
	bool loadScene(const std::string& strfilename, bool preloading = false);
	bool loadPreviewBoot();
	bool destroyBoot();

	bool start() override;
	void onFileChanged(const std::string& strfilename);
	void renderInMenu() override;
	bool inGame() { return !previewEnabled; }
	bool isPreloading() { return _preloadingResources; }
};

