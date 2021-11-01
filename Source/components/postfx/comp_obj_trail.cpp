#include "mcv_platform.h"
#include "comp_obj_trail.h"
#include "comp_fx_stack.h"
#include "render/textures/render_to_texture.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("object_trails", TCompObjectTrail);

TCompObjectTrail::TCompObjectTrail()
{
	bool is_ok = ctes.create(CB_SLOT_FOCUS, "ObjectTrail");
	assert(is_ok);
}

TCompObjectTrail::~TCompObjectTrail()
{
	ctes.destroy();
}

void TCompObjectTrail::load(const json& j, TEntityParseContext& ctx) {

	TCompBilateralBlur::load(j, ctx);

	enabled = j.value("enabled", enabled);
	max_frames = j.value("max_frames", max_frames);

	idx = j.value("priority", idx);

	rt_trail = new CRenderToTexture();
	char rt_name[64];
	sprintf(rt_name, "VolumetricLightShape_%08x", CHandle(this).asUnsigned());
	bool is_ok = rt_trail->createRT(rt_name, static_cast<int>(Render.getWidth()),
		static_cast<int>(Render.getHeight()), DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompObjectTrail::renderTrailFrames, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompObjectTrail::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

CTexture* TCompObjectTrail::renderTrailFrames(CTexture* in_texture, CTexture* last_texture) {

	if (!enabled)
		return in_texture;

	CGpuScope gpu_scope("CompObjectTrail");

	ctes.trail_intensity = 1.f - clampf((float)frames / (float)max_frames, 0.f, 1.f);

	ctes.updateFromCPU();
	ctes.activate();

	auto prev_rt = rt_trail->activateRT(Render.depth_stencil_view);
	assert(prev_rt);
	drawFullScreenQuad("render_trails.pipeline", in_texture);

	// Blur the highlights
	CTexture* blurred = TCompBilateralBlur::apply(rt_trail, nullptr);

	prev_rt->activateRT(Render.depth_stencil_view);
	drawFullScreenQuad("add_trails.pipeline", blurred);

	frames++;

	return prev_rt;
}

void TCompObjectTrail::enable(bool status)
{
	enabled = TCompBilateralBlur::enabled = status;
	frames = 0;
	rt_trail->clear(VEC4::Zero);
}

void TCompObjectTrail::debugInMenu() {

	TCompBilateralBlur::debugInMenu();
	ImGui::Separator();
	ImGui::Checkbox("Trail Enabled", &enabled);
	ImGui::DragInt("Max Frames", &max_frames, 1, 1, 20);
}

void TCompObjectTrail::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}