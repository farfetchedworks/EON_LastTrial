#pragma once

#include "modules/module.h"
#include "entity/entity_parser.h"

class CModuleBoot : public IModule
{
	bool _bootCompleted = false;
	bool _bootReady = false;
	bool _preloadingResources = false;
	bool _previewEnabled = false;
	bool _loadPreview = false;
	int total_entries = 1;

	tbb::tbb_thread* _loadThread = nullptr;

	json jBoot;
	int _lastLoaded = 0;
	int _loaded = 0;

	std::vector< TEntityParseContext > ctxs;

public:

	CModuleBoot(const std::string& name) : IModule(name) { }

	void loadBoot(const std::string& strfilename);
	bool loadScene(const std::string& strfilename, bool preloading = false);
	bool loadPreviewBoot();
	bool destroyBoot();

	bool start() override;
	void update(float dt) override;
	bool customStart();
	void onFileChanged(const std::string& strfilename);
	void renderInMenu() override;
	bool inGame() { return !_previewEnabled; }
	bool isPreloading() { return _preloadingResources; }
	bool ready() { return _bootReady; }
};

