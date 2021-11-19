#include "mcv_platform.h"
#include "comp_buffers.h"
#include "render/compute/gpu_buffer.h"
#include "render/shaders/shader_ctes_manager.h"
#include "components/common/comp_render.h"
#include "utils/data_saver.h"
#include "render/render.h"
#include "../bin/data/shaders/constants_particles.h"

DECL_OBJ_MANAGER("buffers", TCompBuffers);

// -------------------------------------------------------
void from_json(const json& j, CtesCSDemo& ct) {
	ct.cs_demo_amplitude = j.value("amplitude", 1.0f);
	ct.cs_demo_distance = j.value("distance", 1.0f);
	ct.cs_demo_phase = j.value("phase", 0.0f);
}

//void to_json(json& j, const CtesCSDemo& ct) {
//  j = {};
//  j["amplitude"] = ct.cs_demo_amplitude;
//  j["distance"] = ct.cs_demo_distance;
//  j["phase"] = ct.cs_demo_phase;
//}

template<>
bool debugCteInMenu<CtesCSDemo>(CtesCSDemo& ct) {
	bool changed = false;
	changed |= ImGui::DragFloat("Amplitude", &ct.cs_demo_amplitude, 0.01f, 0.01f, 2.0f);
	changed |= ImGui::DragFloat("Distance", &ct.cs_demo_distance, 0.01f, 0.01f, 10.0f);
	changed |= ImGui::DragFloat("Phase", &ct.cs_demo_phase, 0.01f, 0.0f, 6.28f);
	return changed;
}

// -------------------------------------------------------
void from_json(const json& j, CtesParticleSystem& p) {
	p.emitter_time_between_spawns = j.value("emitter_time_between_spawns", 0.02f);
	p.emitter_num_particles_per_spawn = j.value("emitter_num_particles_per_spawn", 50);
	p.emitter_initial_pos = loadVEC3(j, "emitter_initial_pos");
	p.emitter_initial_scale_avg = j.value("emitter_initial_scale_avg", 0.02f);
	p.emitter_initial_scale_range = j.value("emitter_initial_scale_range", 0.02f);
	p.emitter_initial_speed = j.value("emitter_initial_speed", 50.f);
	p.emitter_radius = j.value("emitter_radius", 2.f);
	p.emitter_follow_owner = j.value("emitter_follow_owner", 0.f);
	p.emitter_owner_position = loadVEC3(j, "emitter_owner_position");
	p.emitter_stopped = j.value("emitter_stopped", 0.f);
	p.emitter_frame_size = loadVEC2(j, "emitter_frame_size", VEC2::Zero);

	if (j.count("colors")) {
		const json& jcolors = j["colors"];
		for (size_t i = 0; i < jcolors.size(); ++i)
			p.psystem_colors_over_time[i] = loadColor(jcolors[i]);
	}
	if (j.count("scales")) {
		const json& jvals = j["scales"];
		for (size_t i = 0; i < jvals.size(); ++i)
			p.psystem_sizes_over_time[i] = jvals[i].get<float>() * VEC4(1, 1, 1, 1);
	}
}

template<>
bool debugCteInMenu<CtesParticleSystem>(CtesParticleSystem& c) {
	bool changed = false;
	changed |= ImGui::DragFloat("Time Between Spawns", &c.emitter_time_between_spawns, 0.01f, 0.0f, 2.0f);
	changed |= ImGui::DragInt("# Particles per spawn", (int*)&c.emitter_num_particles_per_spawn, 0.2f, 0, 100);
	changed |= ImGui::DragFloat3("Initial Pos", &c.emitter_initial_pos.x, 0.1f, -50.f, 50.f);
	changed |= ImGui::DragFloat("Initial Scale Avg", &c.emitter_initial_scale_avg, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Initial Scale Range", &c.emitter_initial_scale_range, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Initial Speed", &c.emitter_initial_speed, 0.1f, 0.0f, 100.0f);
	changed |= ImGui::DragFloat("Radius", &c.emitter_radius, 0.1f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Follow owner", &c.emitter_follow_owner, 1.f, 0.0f, 1.f);
	changed |= ImGui::DragFloat3("Owner Position", &c.emitter_owner_position.x, 0.1f, -50.0f, 50.f);
	changed |= ImGui::DragFloat("Stopped", &c.emitter_follow_owner, 1.f, 0.0f, 1.f);

	if (ImGui::TreeNode("Colors...")) {
		for (int i = 0; i < 8; ++i) {
			char title[64];
			sprintf(title, "%d", i);
			changed |= ImGui::ColorEdit4(title, &c.psystem_colors_over_time[i].x);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Scale...")) {
		for (int i = 0; i < 8; ++i) {
			char title[64];
			sprintf(title, "%d", i);
			changed |= ImGui::DragFloat(title, &c.psystem_sizes_over_time[i].x, 0.01f, 0.0f, 1.0f);
		}
		ImGui::TreePop();
	}

	return changed;
}


// ----------------------------------------------------------------------
TCompBuffers::~TCompBuffers() {
	for (auto& b : gpu_buffers)
		b->destroy();
	gpu_buffers.clear();

	for (auto& b : cte_buffers)
		b->destroy();
	cte_buffers.clear();

	for (auto& b : textures)
		b->destroy();
	textures.clear();
}

void TCompBuffers::debugInMenu() {

	for (auto b : gpu_buffers)
		b->debugInMenu();
	for (auto b : cte_buffers)
		b->debugInMenu();
	for (auto b : textures)
		b->renderInMenu();

	if (ImGui::Button("Upload"))
	{
		uploadFromCPU();
	}
}

CGPUBuffer* TCompBuffers::getBufferByName(const char* name) {
	for (auto b : gpu_buffers) {
		if (strcmp(b->name.c_str(), name) == 0)
			return b;
	}
	return nullptr;
}

CTexture* TCompBuffers::getTextureByName(const char* name) {
	for (auto b : textures) {
		if (strcmp(b->getName().c_str(), name) == 0)
			return b;
	}
	return nullptr;
}

CBaseShaderCte* TCompBuffers::getCteByName(const char* name) {
	for (auto b : cte_buffers) {
		if (strcmp(b->getName(), name) == 0)
			return b;
	}
	return nullptr;
}

void TCompBuffers::load(const json& j, TEntityParseContext& ctx) {

	if (!ShaderCtesManager.isDefined("CtesCSDemo")) {
		ShaderCtesManager.registerStruct("CtesCSDemo", CShaderCtesManager::makeFactory<CtesCSDemo>());
		ShaderCtesManager.registerStruct("CtesParticleSystem", CShaderCtesManager::makeFactory<CtesParticleSystem>());
		// ...
	}

	// Owned buffers by me
	for (auto jit : j.items()) {
		const std::string& key = jit.key();
		json jval = jit.value();
		jval["name"] = key;

		CBaseShaderCte* cte = ShaderCtesManager.createCte(key.c_str(), jval);
		if (cte) {
			cte_buffers.push_back(cte);
		}
		else {
			if (jval.is_object()) {
				if (jval.count("create_texture")) {
					const json& jparams = jval["create_texture"];
					int xres = jparams.value("xres", 0);
					int yres = jparams.value("yres", 0);
					assert(xres != 0);
					assert(yres != 0);
					// ...
					DXGI_FORMAT fmt = readFormat(jparams, "format");
					CTexture* texture = new CTexture();
					bool is_ok = texture->create(key.c_str(), xres, yres, fmt, CTexture::CREATE_WITH_COMPUTE_SUPPORT);
					assert(is_ok);
					texture->setResourceName(key);
					textures.push_back(texture);
					continue;
				}
			}

			// Else, conside this is a gpu buffer
			CGPUBuffer* gpu_buffer = new CGPUBuffer();
			bool is_ok = gpu_buffer->create(jval);
			assert(is_ok || fatal("Failed to create gpu buffer %s\n", key.c_str()));
			gpu_buffers.push_back(gpu_buffer);
		}
	}

	uploadFromCPU();
}

void TCompBuffers::update(float dt)
{
	if (dirty)
	{
		uploadFromCPU();
	}
}

void TCompBuffers::uploadFromCPU()
{
	// Initialize buffer with the arguments of an indirect instanced draw call, so
	// copy the number of triangles / vertices from a mesh. The # of instances
	// will be set by another cs.

	for (auto b : gpu_buffers)
	{
		const json& j = b->jData;

		if (b->is_indirect && j.count("init_indirect_from_mesh"))
		{
			std::string mesh_name = j["init_indirect_from_mesh"];
			const CMesh* mesh = Resources.get(mesh_name)->as<CMesh>();
			auto& g = mesh->getGroups();
			assert(g.size() > 0);
			uint32_t args[5] = { g[0].num_indices, 0, g[0].first_index, 0, 0 };
			b->copyCPUtoGPUFrom(args);
		}
	}

	dirty = false;
}