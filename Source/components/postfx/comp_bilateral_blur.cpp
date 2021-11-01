#include "mcv_platform.h"
#include "comp_bilateral_blur.h"
#include "comp_fx_stack.h"
#include "render/draw_primitives.h"
#include "render/textures/render_to_texture.h"

DECL_OBJ_MANAGER("bilateral_blur", TCompBilateralBlur);

TCompBilateralBlur::TCompBilateralBlur()
{
	bool is_ok = cte_blur.create(CB_SLOT_BLUR, "bilateral_blur");
	assert(is_ok);
}

TCompBilateralBlur::~TCompBilateralBlur()
{
	cte_blur.destroy();
}

void TCompBilateralBlur::load(const json& j, TEntityParseContext& ctx) {

	enabled = j.value("enabled", enabled);
	iterations = j.value("iterations", iterations);

	x_offset = j.value("x_offset", x_offset);
	y_offset = j.value("y_offset", y_offset);

	// set apply callback
	apply_fn = std::bind(&TCompBilateralBlur::apply, this, std::placeholders::_1, std::placeholders::_2);

	int xres = static_cast<int>(Render.getWidth() * 0.5);
	int yres = static_cast<int>(Render.getHeight() * 0.5);

	rt_half_y = new CRenderToTexture();
	char rt_name[64];
	sprintf(rt_name, "Bilateral_blur_y%08x", CHandle(this).asUnsigned());
	bool is_ok = rt_half_y->createRT(rt_name, xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
	assert(is_ok);

	rt_output = new CRenderToTexture();
	sprintf(rt_name, "Bilateral_blur_xy%08x", CHandle(this).asUnsigned());
	is_ok = rt_output->createRT(rt_name, xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
	assert(is_ok);
}

void TCompBilateralBlur::onEntityCreated()
{
	// Not used in the stack

	/*TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = 10;
	CHandle(this).getOwner().sendMsg(msg);*/
}

void TCompBilateralBlur::debugInMenu()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::DragInt("Steps", &iterations, 1, 0, 16);
	ImGui::DragFloat("Offset X", &x_offset, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Offset Y", &y_offset, 0.01f, 0.0f, 10.0f);
}

CTexture* TCompBilateralBlur::apply(CTexture* in_texture, CTexture* last_texture)
{
	if (!enabled)
		return in_texture;
	CGpuScope gpu_scope("CompBilateralBlur");

	cte_blur.bt_blur_dummy1 = 0.f;
	cte_blur.bt_blur_dummy2 = 0.f;

	for (int i = 0; i < iterations; ++i)
	{
		// horizontal blur
		cte_blur.bt_blur_offset = VEC2(0, y_offset / (float)rt_half_y->getHeight());
		cte_blur.activate();
		cte_blur.updateFromCPU();
		rt_half_y->activateRT();
		drawFullScreenQuad("bilateral_blur.pipeline", i == 0 ? in_texture : rt_output);
		CTexture::deactivate(TS_ALBEDO);

		// vertical blur
		cte_blur.bt_blur_offset = VEC2(x_offset / (float)rt_half_y->getWidth(), 0);
		cte_blur.activate();
		cte_blur.updateFromCPU();
		rt_output->activateRT();
		drawFullScreenQuad("bilateral_blur.pipeline", rt_half_y);
		CTexture::deactivate(TS_ALBEDO);
	}

	return rt_output;
}

void TCompBilateralBlur::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}