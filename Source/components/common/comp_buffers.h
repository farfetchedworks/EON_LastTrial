#pragma once
#include "comp_base.h"
#include "entity/entity.h"

class CGPUBuffer;

struct TCompBuffers : public TCompBase {

	std::vector< CGPUBuffer* >     gpu_buffers;
	std::vector< CBaseShaderCte* > cte_buffers;
	std::vector< CTexture* >       textures;

	bool dirty = true;
	void uploadFromCPU();

public:

	~TCompBuffers();

	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);

	void debugInMenu();

	CGPUBuffer* getBufferByName(const char* name);
	CBaseShaderCte* getCteByName(const char* name);
	CTexture* getTextureByName(const char* name);

};