#pragma once
#include "mcv_platform.h"
#include "engine.h"
#include "render/render_module.h"
#include "render/draw_primitives.h"
#include "input/input_module.h"
#include "modules/module_boot.h"
#include "modules/game/module_fluid_simulation.h"
#include "geometry/geometry.h"
#include "geometry/camera.h"
#include "render/shaders/pipeline_state.h"
#include "render/compute/compute_shader.h"
#include "render/render_manager.h"
#include "render/compute/gpu_buffer.h"

#include "components/render/comp_irradiance_cache.h"
#include "components/common/comp_light_spot.h"
#include "components/common/comp_particles.h"
#include "components/gameplay/comp_fluid_simulation.h"
#include "components/projectiles/comp_area_delay_projectile.h"
#include "components/projectiles/comp_wave_projectile.h"
#include "components/postfx/comp_color_grading.h"
#include "components/postfx/comp_blur.h"
#include "components/postfx/comp_focus.h"
#include "components/postfx/comp_edge_detector.h"
#include "components/postfx/comp_bloom.h"
#include "components/postfx/comp_godrays.h"
#include "components/postfx/comp_render_reflections.h"
#include "components/postfx/comp_fx_stack.h"

#include "skeleton/comp_skeleton.h"

#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/ImGuizmo.h"
#include "imgui/extra/imgui-knob.h"
#include "imgui/extra/imgui-plotmultilines.h"

#include "draw_primitives.h"

#include "utils/renderdoc_app.h"

RENDERDOC_API_1_4_0* rdoc_api = NULL;

/// --------------------------- TO BE MOVED 
CShaderCte<CtesCamera> cte_camera;
CShaderCte<CtesObject> cte_object;
CShaderCte<CtesLight> cte_light;
CShaderCte<CtesWorld> cte_world;
CShaderCte<CtesUI> cte_ui;
CShaderCte<CtesParticle> cte_particle;
CShaderCte<CtesBlur> cte_blur;
CShaderCte<CtesAreaDelay> cte_area_delay;
CShaderCte<CtesDitherPattern> cte_dither_pattern;

bool CRenderModule::RENDER_IMGUI = false;

/// -----------------------------------
static NamedValues<int>::TEntry output_entries[] = {
  {OUTPUT_COMPLETE, "Complete"},
  {OUTPUT_UNLIT, "Unlit"},
  {OUTPUT_DEPTH, "Depth"},
  {OUTPUT_ALBEDOS, "Albedo"},
  {OUTPUT_NORMALS, "Normals"},
  {OUTPUT_ROUGHNESS, "Roughness"},
  {OUTPUT_METALLIC, "Metallic"},
  {OUTPUT_EMISSIVE, "Emissive"},
  {OUTPUT_BAKED_AO, "Baked AO"},
  {OUTPUT_SCREEN_SPACE_AO, "ScreenSpace AO"},
  {OUTPUT_AO_COMBINED, "AO Combined"},
  {OUTPUT_BEFORE_TONE_MAPPING, "Before Tone Mapping"},
  {OUTPUT_BEFORE_GAMMA_CORRECTION, "Before Gamma Correction"},
};
static NamedValues<int> output_names(output_entries, sizeof(output_entries) / sizeof(NamedValues<int>::TEntry));

/// -----------------------------------

static void ImGuiCreate() {
	PROFILE_FUNCTION("ImGuiCreate");
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	//ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(CApplication::get().getHandle());
	ImGui_ImplDX11_Init(Render.device, Render.ctx);
}

static void ImGuiDestroy() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

static void ImGuiBeginFrame() {
	PROFILE_FUNCTION("ImGui");
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

static void ImGuiEndFrame() {
	PROFILE_FUNCTION("Imgui");
	CGpuScope gpu_scope("Imgui");
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CRenderModule::load(const std::string& filename)
{
	// const json& j = loadJson(filename);
}

bool CRenderModule::start()
{
	ImGuiCreate();

	setClearColor({ 0.f, 0.f, 0.f, 1.0f });

	createAndRegisterPrimitives();

	loadPipelines();

	loadComputeShaders();

	cte_object.create(CB_SLOT_OBJECT, "Object");
	cte_object.object_world = MAT44::Identity;
	cte_camera.create(CB_SLOT_CAMERA, "Camera");
	cte_light.create(CB_SLOT_LIGHT, "Light");
	cte_ui.create(CB_SLOT_UI, "UI");
	cte_particle.create(CB_SLOT_PARTICLES, "Particle");
	cte_blur.create(CB_SLOT_BLUR, "Blur");
	cte_area_delay.create(CB_SLOT_CUSTOM, "Area_Delay");

	cte_world.create(CB_SLOT_WORLD, "World");
	cte_world.ambient_factor = 1.f;
	cte_world.exposure_factor = 1.0f;
	cte_world.output_channel = OUTPUT_COMPLETE;
	cte_world.amount_color_grading = 0.0f;
	cte_world.emissive_irradiance_multiplier = 1.0;
	cte_world.timeReversal_rewinding = 0.f;
	cte_world.timeReversal_rewinding_time = 0.f;
	cte_world.boot_in_preview = 0.f;
	cte_world.in_gameplay = 0.f;
	cte_world.player_dead = 0.f;

	cte_dither_pattern.create(CB_SLOT_DITHER, "Dither");
	cte_dither_pattern.dither_pattern = {
	  0.0f,    0.5f,    0.125f,  0.625f ,
	  0.75f,   0.22f,   0.875f,  0.375f ,
	  0.1875f, 0.6875f, 0.0625f, 0.5625 ,
	  0.9375f, 0.4375f, 0.8125f, 0.3125
	};
	cte_dither_pattern.updateFromCPU();

	int xres = (int)Render.getWidth();
	int yres = (int)Render.getHeight();

	bool is_ok = true;
	is_ok &= deferred_renderer.create(xres, yres);
	assert(is_ok);

	rt_deferred_output = new CRenderToTexture;
	is_ok &= rt_deferred_output->createRT("rt_deferred_output.dds", xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN, true);

	_blackHole = new CTexture;
	is_ok &= _blackHole->create("black_hole.dds", xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT, CTexture::CREATE_DYNAMIC);
	Resources.registerResource(_blackHole, "rt_black_hole.dds", getClassResourceType<CTexture>());

	_lastOutput = new CTexture;
	is_ok &= _lastOutput->create("last_rt_final.dds", xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT, CTexture::CREATE_DYNAMIC);

	rt_final = new CRenderToTexture;
	is_ok &= rt_final->createRT("rt_final.dds", xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN, true);

	assert(is_ok);

	json j = loadJson("data/adaptative_exposure.json");

	// Read a bunch of compute shaders and gpu buffers
	TEntityParseContext ctx;
	comp_exposure.load(j["compute"], ctx);

	exposure_histogram = new CGPUBuffer();
	exposure_histogram->create(sizeof(uint), 256, true, "exposure_histogram");

	exposure_avg_texture = new CTexture();
	exposure_avg_texture->create("exposure_avg_texture", 1, 1, DXGI_FORMAT_R32_FLOAT, CTexture::CREATE_WITH_COMPUTE_SUPPORT);

	//HMODULE mod;
	//HINSTANCE hinstLib = LoadLibrary("renderdoc.dll");
	//if (hinstLib != NULL && (mod = GetModuleHandleA("renderdoc.dll")))
	//{
	//	pRENDERDOC_GetAPI RENDERDOC_GetAPI =
	//		(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
	//	int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
	//	assert(ret == 1);
	//}

	return true;
}

void CRenderModule::update(float dt)
{
#ifndef _DEBUG
	if (CEngine::get().getInput(input::MENU)->getKey('I').getsPressed())
	{
		RENDER_IMGUI = !RENDER_IMGUI;
		CApplication::get().setWndMouseVisible(RENDER_IMGUI);
	}
#endif // _DEBUG
}

void CRenderModule::createPipelineFromJson(const std::string& name, const json& jdef)
{
	TFileContext file_context(name);
	PROFILE_FUNCTION(name);
	const CResourceType* pipeline_type = getClassResourceType<CPipelineState>();
	// Check if we need to create a new pipeline object or we are rereading an existing one
	CPipelineState* pipeline = nullptr;
	auto it = my_pipelines.find(name);
	if (it == my_pipelines.end())
	{
		// Not found... create a new one
		pipeline = new CPipelineState();
		my_pipelines[name] = pipeline;
		Resources.registerResource(pipeline, name, pipeline_type);
	}
	else
		pipeline = it->second;

	// Let the pipeline reconfigure itself from the json definition
	pipeline->create(jdef);
}

void CRenderModule::loadPipelines() {
	PROFILE_FUNCTION("loadPipelines");
	// This needs to go somewhere else
	json j = loadJson("data/pipelines.json");

	const CResourceType* pipeline_type = getClassResourceType<CPipelineState>();

	// Register all resource types defined in the json
	for (auto it : j.items()) {
		const std::string& key = it.key();
		const json& jdef = it.value();
		std::string name = key + pipeline_type->getExtension();

		createPipelineFromJson(name, jdef);

		// Once the pipeline has been configured....
		if (jdef.value("add_skin_support", false))
		{
			json jdef_new = jdef;
			std::string vs_entry = jdef.value("vs_entry", "VS");
			jdef_new["vs_entry"] = vs_entry + "_Skin";
			std::string vs_decl = jdef["vertex_decl"];
			jdef_new["vertex_decl"] = vs_decl + "Skin";
			std::string new_name = key + "_skin" + pipeline_type->getExtension();
			jdef_new.erase("add_skin_support");
			jdef_new.erase("add_instanced_support");
			jdef_new["use_skin"] = true;
			if (jdef.count("z_skin")) {
				jdef_new["z"] = jdef["z_skin"];
				jdef_new.erase("z_skin");
			}
			createPipelineFromJson(new_name, jdef_new);
		}

		// Once the pipeline has been configured....
		if (jdef.value("add_instanced_support", false))
		{
			json jdef_new = jdef;
			std::string vs_entry = jdef.value("vs_entry", "VS");
			jdef_new["vs_entry"] = vs_entry + "_Instanced";
			std::string new_name = key + "_instanced" + pipeline_type->getExtension();
			jdef_new.erase("add_skin_support");
			jdef_new.erase("add_instanced_support");
			jdef_new["use_instanced"] = true;
			createPipelineFromJson(new_name, jdef_new);
		}

	}
}

void CRenderModule::loadComputeShaders()
{
	PROFILE_FUNCTION("loadCompute");
	// This needs to go somewhere else
	json j = loadJson("data/computes.json");

	const CResourceType* compute_type = getClassResourceType<CComputeShader>();
	// Register all resource types defined in the json
	for (auto it : j.items())
	{
		const std::string& key = it.key();
		const json& jdef = it.value();
		std::string name = key + compute_type->getExtension();

		TFileContext file_context(name);
		PROFILE_FUNCTION(name);
		// Check if we need to create a new pipeline object or we are rereading an existing one
		CComputeShader* compute = new CComputeShader();
		compute->create(jdef);
		Resources.registerResource(compute, name, compute_type);
	}
}

void CRenderModule::onFileChanged(const std::string& filename) {
	if (filename == "data/pipelines.json")
		loadPipelines();

	// Reload pipelines
	if (filename.find("data/shaders/") != std::string::npos)
		loadPipelines();
}

void CRenderModule::activateMainCamera() {
	CEntity* e_camera = CEngine::get().getRender().getActiveCamera();
	if (!e_camera) {
		e_camera = getEntityByName("camera_mixed");
	}
	if (e_camera) {
		assert(e_camera);
		TCompCamera* active_camera = e_camera->get<TCompCamera>();
		assert(active_camera);
		active_camera->setAspectRatio((float)Render.getWidth() / (float)Render.getHeight());
		activateCamera(*active_camera);

		TCompColorGrading* c_color_grading = e_camera->get<TCompColorGrading>();
		if (c_color_grading) {
			c_color_grading->lut->activate(TS_LUT_COLOR_GRADING);
			cte_world.amount_color_grading = c_color_grading->getAmount();
		}
	}
}

void CRenderModule::startRenderDocCapture()
{
	if (rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);
}

void CRenderModule::stopRenderDocCapture()
{
	if (rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);
}

void CRenderModule::updateWorldCtes(float width, float height) {
	cte_world.render_width = width;
	cte_world.render_height = height;
	cte_world.inv_render_width = 1.0f / cte_world.render_width;
	cte_world.inv_render_height = 1.0f / cte_world.render_height;
	cte_world.world_time = (float)Time.current;
	cte_world.world_delta_time = (float)Time.delta;
	cte_world.updateFromCPU();
}

void CRenderModule::updateAllCtesBones() {
	getObjectManager<TCompSkeleton>()->forEach([](TCompSkeleton* c) {
		c->updateCteBones();
		});
}

void CRenderModule::generateShadowMaps() {
	CGpuScope gpu_scope("shadowMaps");
	getObjectManager<TCompLightSpot>()->forEach([](TCompLightSpot* light_spot) {
		light_spot->generateShadowMap();
		});
}

void CRenderModule::renderAll()
{
	CTexture::deactivateCS(TS_DEFERRED_OUTPUT);
	CTexture::deactivate(TS_DEFERRED_OUTPUT);

	generateShadowMaps();

	activateMainCamera();

	EngineFluidSimulation.activateFluids();

	CEntity* e_camera = CEngine::get().getRender().getActiveCamera();
	deferred_renderer.render(rt_deferred_output, e_camera);

	EngineFluidSimulation.deactivateFluids();

	rt_final->activateRT();
	rt_deferred_output->activate(TS_DEFERRED_OUTPUT);
	rt_deferred_output->activateCS(TS_DEFERRED_OUTPUT);

	drawFullScreenQuad("deferred_resolve.pipeline", rt_deferred_output);

	getObjectManager<TCompIrradianceCache>()->forEach([](TCompIrradianceCache* irradiance) {
		irradiance->renderProbes();
	});

	exposure_avg_texture->activate(TS_EXPOSURE);

	RenderManager.renderAll(eRenderChannel::RC_DISTORSIONS, e_camera);
	RenderManager.renderAll(eRenderChannel::RC_TRANSPARENT, e_camera);

	// Render black holes
	_blackHole->copyFromResource(rt_final);
	deferred_renderer.renderBlackHoles(rt_final, _blackHole);

	rt_final->activateRT();

	// Render projectile effects
	_lastOutput->copyFromResource(rt_final); 

	TCompAreaDelayProjectile::renderAll(_lastOutput);
	TCompWaveProjectile::renderAll(_lastOutput);

	CTexture::deactivate(TS_EXPOSURE);

	CTexture* curr_output = rt_final;

	// -- Now starting the final render presentation
	if (e_camera)
	{
		TCompFXStack* stack = e_camera->get<TCompFXStack>();
		if (stack) {
			curr_output = stack->render(curr_output, _lastOutput);
		}
	}

	// ----

	Render.activateBackBuffer();

	// Compute adaptative exposure
	{
		CGpuScope gpu_scope("Adaptative Exposure");

		cte_world.activateCS();

		// Compute histogram
		exposure_histogram->activateCSasUAV(0);
		curr_output->activateCS(0);

		comp_exposure.executions[0].sizes[0] = (uint32_t)cte_world.render_width;
		comp_exposure.executions[0].sizes[1] = (uint32_t)cte_world.render_height;

		comp_exposure.executions[0].run(nullptr);

		// Compute exposure average
		exposure_avg_texture->activateCSasUAV(1);
		comp_exposure.executions[1].run(nullptr);

		ID3D11UnorderedAccessView* nullUAV = nullptr;
		Render.ctx->CSSetUnorderedAccessViews(1, 1, &nullUAV, nullptr);
	}

	// HDR a ToneMapped+Gamma+LDR
	drawFullScreenQuad("presentation.pipeline", curr_output, exposure_avg_texture);

	deferred_renderer.getDepthFBO()->activateCS(TS_DEFERRED_LINEAR_DEPTH);
}

void CRenderModule::generateFrame()
{
	PROFILE_FUNCTION("generateFrame");
	ImGuiBeginFrame();

	cte_camera.activate();
	cte_object.activate();
	cte_light.activate();
	cte_world.activate();
	cte_ui.activate();
	cte_dither_pattern.activate();

	updateWorldCtes((float)Render.getWidth(), (float)Render.getHeight());

	// Give an opportunity to all the modules to render something
	CModuleManager& modules = CEngine::get().getModuleManager();

	if (modules.inGamestate("playing") 
		|| modules.inGamestate("intro")
		|| modules.inGamestate("settings"))
	{
		renderAll();
		modules.render();
		modules.renderDebug();
	}
	else
	{
		Render.activateBackBuffer();
	}

	modules.renderUI();
	modules.renderUIDebug();

	CModuleBoot& boot = CEngine::get().getBoot();

	activateMainCamera();

	// Any tool inside the engine wants to render imgui
	if (boot.inGame() && !RENDER_IMGUI) {
#ifdef _DEBUG
		modules.renderInMenu();
#endif
	}
	else {
		modules.renderInMenu();
	}

	ImGuiEndFrame();
	Render.swapFrames();
}

void CRenderModule::stop() {
	SAFE_DESTROY(rt_deferred_output);
	SAFE_DESTROY(_blackHole);
	SAFE_DESTROY(_lastOutput);
	SAFE_DESTROY(rt_final);
	SAFE_DESTROY(exposure_avg_texture);
	SAFE_DESTROY(exposure_histogram);
	cte_camera.destroy();
	cte_world.destroy();
	cte_light.destroy();
	cte_area_delay.destroy();
	cte_object.destroy();
	cte_ui.destroy();
	cte_blur.destroy();
	cte_particle.destroy();
	ImGuiDestroy();
	deferred_renderer.destroy();
}

void CRenderModule::renderInMenu() {
	RenderManager.renderInMenu();
	Resources.renderInMenu();

	if (ImGui::TreeNode("Performance")) {
		ImGui::Columns(2);
		ImGui::SetColumnWidth(-1, 55);
		ImGui::Knob("##timescale", &Time.scale_factor, 0.f, 2.f, 1.f, true);

		ImGui::NextColumn();
		ImGui::Text("DT scaled %.1fms", Time.delta * 1000.f);
		ImGui::SameLine();
		ImGui::Text("DT Unscaled %.1fms", Time.delta_unscaled * 1000.f);
		ImGui::Text("FPS: %.0f ", 1.f / Time.delta_unscaled);
		ImGui::Text("Time: %lf", Time.current);
		ImGui::Columns(1);

		static const int nsamples = 250;
		auto size = ImGui::GetContentRegionAvail();
		size.y = 60;

		static std::vector<std::vector<float>> data(2, std::vector<float>(nsamples, 0));
		static std::vector<float> sdt_values(nsamples, 0);
		static std::vector<float> dt_values(nsamples, 0);
		std::rotate(sdt_values.begin(), sdt_values.begin() + 1, sdt_values.end());
		std::rotate(dt_values.begin(), dt_values.begin() + 1, dt_values.end());
		sdt_values[nsamples - 1] = Time.delta_unscaled;
		dt_values[nsamples - 1] = Time.delta;
		data[0] = sdt_values;
		data[1] = dt_values;

		std::vector<std::string> labels = { "", "" };
		std::vector<ImColor> colors = { ImColor(255,0,0) , ImColor(0,255,0) };
		float min = *std::min_element(sdt_values.begin(), sdt_values.end()) - 0.01f;
		float max = *std::max_element(sdt_values.begin(), sdt_values.end()) + 0.01f;
		ImGui::PlotMultiLines(data, "", labels, colors, max, min, size);

		ImGui::Separator();
		static int nframes = 3;
		ImGui::DragInt("NumFrames To Capture", &nframes, 0.1f, 1, 20);
		if (ImGui::SmallButton("Start CPU Trace...")) {
			PROFILE_SET_NFRAMES(nframes);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Globals")) {


		ImGui::DragFloat("Ambient Factor", &cte_world.ambient_factor, 0.01f, 0.f, 1.0f);
		// ImGui::DragFloat("Exposure Factor", &cte_world.exposure_factor, 0.01f, 0.f, 5.0f);
		ImGui::TreePop();
	}

	output_names.debugInMenu("Output", cte_world.output_channel);
}