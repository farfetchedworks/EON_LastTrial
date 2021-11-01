#pragma once
#include "components/common/comp_base.h"
#include "render/textures/render_to_texture.h"
#include "render/deferred_renderer.h"
#include "components/messages.h"

class CFloatCubemap;
class CGPUBuffer;

//struct to store probes
struct sProbe {
    VEC3 pos;               // where is located
    VEC3 local;             // its ijk pos in the matrix
    bool disabled = false;  // should be used?
};

struct sIrrHeader {
    VEC3 start;
    VEC3 end;
    VEC3 delta;
    VEC3 dims;
    int num_probes;
};

class TCompIrradianceCache : public TCompBase
{
public:
	DECL_SIBLING_ACCESS();

private:
    std::string irradianceFilename;

    HWND hwndPB;    // Handle of progress bar.

	VEC3 grid_density;
	VEC3 grid_dimensions;
	VEC3 delta;
	VEC3 start_pos;
	VEC3 end_pos;

    int cache_idx = 0;
    int cubemap_batch_size = 256;
    int visible_probes_size = 0;

    std::vector<sProbe> probes;
    VEC4 cubeMapVecs[IR_SIZE * IR_SIZE * 6];

    CHandle h_transform;
    CDeferredRenderer deferred_renderer;
    CRenderToTexture* render_texture = nullptr;
    CTexture* read_texture = nullptr;

    CTexture* sh_texture = nullptr;
    CGPUBuffer* cubemaps_vectors = nullptr;
    CGPUBuffer* probe_idx = nullptr;
    CGPUBuffer* probe_weights = nullptr;

    static float EMISSIVE_MULTIPLIER;

    //bool use_texture = true;
    bool use_disabled = true;

public:

    static bool GLOBAL_RENDER_PROBES;
    static bool RENDERING_ACTIVE;
    static float GLOBAL_IRRADIANCE_MULTIPLIER;

    static void setRenderingIrradianceEnabled(bool enabled);

	void load(const json& j, TEntityParseContext& ctx);
    void initCache(int idx, CGPUBuffer** sh_buffer);
    void showInMenu(int idx, CGPUBuffer* sh_buffer);

	void renderProbes();
    void updateWorldVars();

    bool saveToFile(CGPUBuffer* sh_buffer);
    bool loadFromFile(CGPUBuffer* sh_buffer);

    // "Offline"
    void createGrid();
    void renderProbeCubemaps(int idx, CGPUBuffer* sh_buffer);
};
