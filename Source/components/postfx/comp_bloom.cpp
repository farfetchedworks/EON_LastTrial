#include "mcv_platform.h"
#include "comp_bloom.h"
#include "render/textures/render_to_texture.h"
#include "comp_fx_stack.h"
#include "render/blur_step.h"

DECL_OBJ_MANAGER("bloom", TCompBloom);

// https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/

// ---------------------
TCompBloom::TCompBloom()
{
	bool is_ok = cte_bloom.create(CB_SLOT_CUSTOM, "bloom");
	assert(is_ok);
}

TCompBloom::~TCompBloom()
{
	cte_bloom.destroy();

	for (auto t : texture_pool)
		SAFE_DESTROY(t);
}

void TCompBloom::debugInMenu()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::DragFloat("Threshold Min", &threshold_min, 0.01f, 0.f, 10.f);
	ImGui::DragFloat("Multiplier", &multiplier, 0.01f, 0.f, 10.f);
	ImGui::DragFloat("Delta", &delta, 0.01f, 0.f, 5.f);
	ImGui::DragFloat("Persistence", &persistence, 0.01f, 0.f, 5.f);
	ImGui::DragInt("Iterations", &iterations, 1, 1, 8);
}

void TCompBloom::load(const json& j, TEntityParseContext& ctx)
{
	threshold_min = j.value("threshold_min", threshold_min);
	iterations = j.value("iterations", iterations);
	multiplier = j.value("multiplier", multiplier);
	persistence = j.value("persistence", persistence);
	delta = j.value("delta", delta);

	idx = j.value("priority", idx);

	pipeline_filter = Resources.get("bloom_filter.pipeline")->as<CPipelineState>();
	pipeline_scale = Resources.get("bloom_scale.pipeline")->as<CPipelineState>();
	pipeline_add = Resources.get("bloom_add.pipeline")->as<CPipelineState>();
	mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();

	// set apply callback
	apply_fn = std::bind(&TCompBloom::generateHighlights, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompBloom::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

CTexture* TCompBloom::generateHighlights(CTexture* in_texture, CTexture* last_texture)
{
	if (!enabled)
		return in_texture;

	CGpuScope gpu_scope("BLOOM");

	int width = Render.getWidth();
	int height = Render.getHeight();

	cte_bloom.bloom_threshold_min = threshold_min;
	cte_bloom.bloom_multiplier = multiplier;
	cte_bloom.bloom_persistence = persistence;
	cte_bloom.updateFromCPU();
	cte_bloom.activate();

	std::vector<CRenderToTexture* > textures(iterations);
	CRenderToTexture* currentDestination = textures[0] = getTemporary(width, height);

	// filter pass
	auto prev_rt = in_texture->blit(currentDestination, pipeline_filter);

	// -------------------------
	CRenderToTexture* currentSource = currentDestination;

	float w = (float)width;
	float h = (float)height;

	//downscale
	cte_bloom.bloom_delta = delta; // 1
	int i = 1;
	for (; i < iterations; i++) {
		w = round(w / 2);
		if (int(h) > 1) {
			h = round(h / 2);
		}
		if (w < 2) {
			break;
		}

		// fill textures[]
		currentDestination = textures[i] = getTemporary((int)w, (int)h);
		cte_bloom.bloom_texel_size = VEC4(1.f / currentSource->getWidth(), 1.f / currentSource->getHeight(), 0.f, 0.f);
		cte_bloom.updateFromCPU();

		currentSource->blit(currentDestination, pipeline_scale);
		currentSource = currentDestination;
	}

	//upscale and blend
	cte_bloom.bloom_multiplier = persistence;
	cte_bloom.bloom_delta = 0.5f;
	for (
		i -= 2;
		i >= 0;
		i-- // i-=2 =>  -1 to point to last element in array, -1 to go to texture above
		) {

		currentDestination = textures[i];
		cte_bloom.bloom_texel_size = VEC4(1.f / currentSource->getWidth(), 1.f / currentSource->getHeight(), 0.f, 0.f);
		cte_bloom.updateFromCPU();

		currentDestination->activateRT();
		pipeline_scale->activate();
		currentSource->activate(TS_ALBEDO);
		mesh->render();

		currentSource = currentDestination;
	}

	//final composition
	prev_rt->activateRT();
	pipeline_add->activate();

	cte_bloom.bloom_multiplier = multiplier;
	cte_bloom.bloom_delta = delta;
	// cte_bloom.bloom_texel_size = VEC4(1.f / prev_rt->getWidth(), 1.f / prev_rt->getHeight(), 0.f, 0.f);
	cte_bloom.updateFromCPU();

	currentSource->activate(0);
	mesh->render();


	// -------------------------
	releaseTemporary(currentSource);
	for (auto* t : textures)
		releaseTemporary(t);

	return prev_rt;
}

void TCompBloom::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}

CRenderToTexture* TCompBloom::getTemporary(int width, int height)
{
	for (auto it = texture_pool.begin(); it != texture_pool.end(); ++it)
	{
		CRenderToTexture* t = (*it);

		if (t->getWidth() == width && t->getHeight() == height)
		{
			texture_pool.erase(it);
			t->setPoolId(0);
			return t;
		}
	}

	// not in pool, create it
	textures_created++;
	CRenderToTexture* texture = new CRenderToTexture();
	texture->createRT(("bloom_temporary_" + std::to_string(textures_created)).c_str(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
	texture->setPoolId(0.f);
	return texture;
}

void TCompBloom::releaseTemporary(CRenderToTexture* texture)
{
	if (!texture)
		return;

	size_t pool_size = texture_pool.size();

	if (texture->getPoolId() != 0)
		return; // texture already in pool

	texture->setPoolId((float)Time.current);
	texture_pool.push_back(texture);

	if (pool_size > 10)
	{
		std::sort(texture_pool.begin(), texture_pool.end(), [](const CRenderToTexture* lhs, const CRenderToTexture* rhs) {
			return lhs->getPoolId() < rhs->getPoolId();
		});

		// destroy last texture
		SAFE_DESTROY(texture_pool[pool_size - 1]);

		// remove from array
		texture_pool.pop_back();
	}
}