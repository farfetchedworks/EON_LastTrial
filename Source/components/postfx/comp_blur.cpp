#include "mcv_platform.h"
#include "comp_blur.h"
#include "comp_fx_stack.h"
#include "render/blur_step.h"

DECL_OBJ_MANAGER("blur", TCompBlur);

void  TCompBlur::load(const json& j, TEntityParseContext& ctx) {
	enabled = j.value("enabled", enabled);
	global_distance = j.value("global_distance", global_distance);
	distance_factors = VEC4(1, 2, 3, 0);      // .w is not used 

	idx = j.value("priority", idx);

	weights.x = j.value("w0", 1.f);
	weights.y = j.value("w1", 1.f);
	weights.z = j.value("w2", 1.f);
	weights.w = j.value("w3", 1.f);

	if (j.value("box_filter", false))
		weights = VEC4(1, 1, 1, 1);
	else if (j.value("gauss_filter", false))
		weights = VEC4(70, 56, 28, 8);
	else if (j.count("preset1"))
		setPreset1();
	else if (j.count("preset2"))
		setPreset1();

	/*
				  1
				1   1
			  1   2   1
			1   3   3   1
		  1   4   6   4   1
		1   5   10 10   5   1
	  1   6   15  20  15  6   1
	1   7   21  35  35  21  7   1
  1   8   28  56  70  56  28  8   1   <-- Four taps, discard the last 1
  */


	int xres = Render.getWidth();
	int yres = Render.getHeight();
	std::string rt_name = j.value("rt_name", "Blur");

	int nsteps = j.value("max_steps", 2);
	for (int i = 0; i < nsteps; ++i)
	{
		CBlurStep* s = new CBlurStep();
		char blur_name[64];
		static int g_blur_counter = 0;
		sprintf(blur_name, "%s_%02d", rt_name.c_str(), g_blur_counter);
		g_blur_counter++;

		bool is_ok = s->create(blur_name, xres, yres, DXGI_FORMAT_R8G8B8A8_UNORM);
		assert(is_ok);

		steps.push_back(s);
		xres /= 2;
		yres /= 2;
	}
	nactive_steps = nsteps;

	// set apply callback
	apply_fn = std::bind(&TCompBlur::apply, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompBlur::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

void TCompBlur::setPreset1()
{
	weights = VEC4(70, 56, 28, 8);
	distance_factors = VEC4(1, 2, 3, 4);
	global_distance = 2.7f;
}

void TCompBlur::setPreset2()
{
	weights = VEC4(70, 56, 28, 8);
	distance_factors = VEC4(1, 2, 3, 4);
	global_distance = 2.0f;
}

void  TCompBlur::debugInMenu()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::DragInt("# Steps", &nactive_steps, 0.1f, 0, (int)steps.size());

	ImGui::InputFloat("Weights Center", &weights.x);
	ImGui::InputFloat("Weights 1st", &weights.y);
	ImGui::InputFloat("Weights 2nd", &weights.z);
	ImGui::InputFloat("Weights 3rd", &weights.w);

	if (ImGui::SmallButton("box")) {
		distance_factors = VEC4(1, 2, 3, 4);
		weights = VEC4(1, 1, 1, 1);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("gauss")) {
		weights = VEC4(70, 56, 28, 8);
		distance_factors = VEC4(1, 2, 3, 0);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("linear")) {
		// This is a 5 taps kernel (center + 2 taps on each side)
		// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
		weights = VEC4(0.2270270270f, 0.3162162162f, 0.0702702703f, 0.f);
		distance_factors = VEC4(1.3846153846f, 3.2307692308f, 0.f, 0.f);
	}
	if (ImGui::SmallButton("Preset1"))
		setPreset1();
	ImGui::SameLine();
	if (ImGui::SmallButton("Preset2"))
		setPreset2();


	ImGui::DragFloat("global_distance", &global_distance, 0.01f, 0.1f, 8.0f);
	ImGui::InputFloat("Distance 2nd Tap", &distance_factors.x);
	ImGui::InputFloat("Distance 3rd Tap", &distance_factors.y);
	ImGui::InputFloat("Distance 4th Tap", &distance_factors.z);
	if (ImGui::SmallButton("Default Distances"))
	{
		distance_factors = VEC4(1, 2, 3, 0);
	}

}

CTexture* TCompBlur::apply(CTexture* in_texture, CTexture* last_texture)
{
	if (!enabled)
		return in_texture;
	CGpuScope gpu_scope("CompBlur");

	CTexture* output = in_texture;
	for (int i = 0; i < nactive_steps; ++i) {
		output = steps[i]->apply(output, global_distance, distance_factors, weights);
	}
	return output;
}

void TCompBlur::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}