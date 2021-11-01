#include "mcv_platform.h"
#include "comp_motion_blur.h"
#include "engine.h"
#include "render/textures/render_to_texture.h"
#include "comp_fx_stack.h"
#include "render/render_module.h"

DECL_OBJ_MANAGER("motion_blur", TCompMotionBlur);

extern CShaderCte<CtesCamera> cte_camera;
extern CShaderCte<CtesWorld> cte_world;

TCompMotionBlur::TCompMotionBlur()
{
	bool is_ok = ctes.create(CB_SLOT_FOCUS, "Motion Blur");
	assert(is_ok);
}

TCompMotionBlur::~TCompMotionBlur()
{
	ctes.destroy();
}

void TCompMotionBlur::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

void TCompMotionBlur::load(const json& j, TEntityParseContext& ctx)
{
	enabled = j.value("enabled", enabled);
	blend_in = j.value("blend_in", blend_in);
	blend_out = j.value("blend_out", blend_out);
	ctes.motion_blur_intensity = 1.f;
	ctes.motion_blur_lerp_value = 0.f;
	ctes.motion_blur_dummy = 0.f;
	ctes.motion_blur_dummy1 = 0.f;

	idx = j.value("priority", idx);

	pipeline = Resources.get("motion_blur.pipeline")->as<CPipelineState>();
	mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();

	rt = new CRenderToTexture;
	bool is_ok = rt->createRT("RT_MotionBlur", Render.getWidth(), Render.getHeight(), DXGI_FORMAT_R16G16B16A16_FLOAT);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompMotionBlur::apply, this, std::placeholders::_1, std::placeholders::_2);

	fadein = &interpolators::quartInInterpolator;
	fadeout = &interpolators::quadOutInterpolator;

	ctes.previous_camera_view_projection = cte_camera.camera_view_projection;
}

void TCompMotionBlur::enable()
{
	enabled = true;
	timer = 0.f;
}

void TCompMotionBlur::disable()
{
	enabled = false;
	timer = blend_out;
}

void TCompMotionBlur::debugInMenu()
{
	if (ImGui::Checkbox("Enabled", &enabled))
	{
		if (enabled)
			enable();
		else
			disable();
	}

	ImGui::DragFloat("Intensity", &ctes.motion_blur_intensity, 0.1f, 0.f, 10.f);
	ImGui::Text("Timer %f", timer);
}

CTexture* TCompMotionBlur::apply(CTexture* input_texture, CTexture* last_texture)
{
	if (!enabled && timer < 0.f)
		return input_texture;

	CGpuScope gpu_scope("CompMotionBlur");

	float f = 0.f;

	if (enabled)
	{
		timer += Time.delta_unscaled;
		f = fadein->blend(0.f, 1.f, clampf(timer / blend_in, 0.f, 1.f));
	}
	else
	{
		timer -= Time.delta_unscaled;
		f = fadeout->blend(0.f, 1.f, clampf(timer / blend_out, 0.f, 1.f));
	}

	ctes.motion_blur_lerp_value = f;

	cte_world.exposure_factor = enabled ? lerp(1.f, 0.2f, f) : lerp(1.f, 0.2f, f);

	ctes.updateFromCPU();
	ctes.activate();

	assert(mesh);
	assert(pipeline);
	assert(input_texture);

	rt->activateRT();
	input_texture->activate(TS_ALBEDO);
	pipeline->activate();
	mesh->activate();
	mesh->render();

	ctes.previous_camera_view_projection = cte_camera.camera_view_projection;

	return rt;
}

void TCompMotionBlur::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}