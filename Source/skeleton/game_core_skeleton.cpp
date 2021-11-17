#include "mcv_platform.h"
#include "game_core_skeleton.h"
#include "animation/animation_callback.h"
#include "render/meshes/mesh_io.h"
#include "render/shaders/shader_cte.h"
#include "animation/callbacks/event_callback.h"

#ifdef NDEBUG
#pragma comment(lib, "cal3d.lib" )
#else
#pragma comment(lib, "cal3dd.lib" )
#endif

// To convert from/to cal3d/dx
CalVector DX2Cal(VEC3 p);
CalQuaternion DX2Cal(QUAT q);
VEC3 Cal2DX(CalVector p);
QUAT Cal2DX(CalQuaternion q);
MAT44 Cal2DX(CalVector trans, CalQuaternion rot);

// --------------------- Resource --------------------------
class CGameCoreSkeletonResourceType : public CResourceType {
public:
  const char* getExtension(int idx) const override { return ".skeleton"; }
  const char* getName() const override {
    return "Skeleton";
  }
  IResource* create(const std::string& name) const override {
    dbg("Creating skeleton resource %s\n", name.c_str());
    CGameCoreSkeleton* res = new CGameCoreSkeleton(name);
    bool is_ok = res->create(name);
    assert(is_ok);
    return res;
  }
};

// A specialization of the template defined at the top of this file
// If someone class getResourceTypeForClass<CGameCoreSkeletonResourceClass>, use this function:
template<>
CResourceType* getClassResourceType<CGameCoreSkeleton>() {
	static CGameCoreSkeletonResourceType resource_type;
	return &resource_type;
}

struct TSkinVertex
{
	VEC3 pos;
	VEC3 normal;
	VEC2 uv;
	VEC4 tangent;
	// 8 bytes for the skinning info
	uint8_t bone_ids[4];
	uint8_t weights[4];
};

// Convert a cal mesh into an engine mesh
void CGameCoreSkeleton::convertCalMeshToEngineMesh(const std::string& out_mesh_name, int mesh_id) {
	PROFILE_FUNCTION("CalMesh2Engine");

	// 1 cal core mesh include N sub core meshes, 1 per material
	CalCoreMesh* core_mesh = this->getCoreMesh(mesh_id);
	assert(core_mesh);

	VMeshGroups      groups;
	int nsubmeshes = core_mesh->getCoreSubmeshCount();

	// Compute total vtxs and face numbers
	int total_vtxs = 0;
	int total_faces = 0;
	for (int idx_sm = 0; idx_sm < nsubmeshes; ++idx_sm)
	{
		CalCoreSubmesh* cal_sm = core_mesh->getCoreSubmesh(idx_sm);
		int num_faces = cal_sm->getFaceCount();;

		TMeshGroup group;
		group.first_index = total_faces * 3;
		group.num_indices = num_faces * 3;
		groups.push_back(group);

		total_vtxs += cal_sm->getVertexCount();
		total_faces += num_faces;
	}

	CMemoryDataSaver mds_vtxs;
	CMemoryDataSaver mds_idxs;

	// Counts the total number of vertices already pushed to the global array of vertices in the new mesh
	int acc_vtxs = 0;

	// Convert all vertices from cal 
	for (int idx_sm = 0; idx_sm < nsubmeshes; ++idx_sm)
	{
		CalCoreSubmesh* cal_sm = core_mesh->getCoreSubmesh(idx_sm);

		assert(!cal_sm->getVectorVectorTextureCoordinate().empty() || fatal("The mesh %s has been exported without texture coordinates. Ensure the mesh has a material with a texture assigned when exporting the cmf in max.", core_mesh->getFilename().c_str()));

		int num_vids = cal_sm->getVertexCount();

		// Cal3d has an array of vectors of tex coords, supports more than 1 tex coord per vertex
		auto& cal_all_uvs = cal_sm->getVectorVectorTextureCoordinate();
		if (cal_all_uvs.empty()) {
			fatal("The cal3d mesh %s does NOT have any texture coordinates\n", out_mesh_name.c_str());
			cal_all_uvs.resize(1);
			cal_all_uvs[0].resize(num_vids);
		}

		// Ensure cal3d provides tangent space information for our vertexs
		cal_sm->enableTangents(0, true);
		auto& cal_tangents = cal_sm->getVectorVectorTangentSpace()[0];

		// Array with all the cal vertices in an array
		auto& cal_vtxs = cal_sm->getVectorVertex();
		for (int vid = 0; vid < num_vids; ++vid)
		{
			const CalCoreSubmesh::Vertex& cal_vtx = cal_vtxs[vid];

			// 
			TSkinVertex skin_vtx;
			skin_vtx.pos = Cal2DX(cal_vtx.position);
			skin_vtx.normal = Cal2DX(cal_vtx.normal);

			// Copy the 1st texture coordinate associated to this vertex
			skin_vtx.uv.x = cal_all_uvs[0][vid].u;
			skin_vtx.uv.y = cal_all_uvs[0][vid].v;

			// Weights
			size_t max_engine_influences = 4;
			size_t num_cal_influences = std::min(cal_vtx.vectorInfluence.size(), max_engine_influences);
			for (size_t ninfluence = 0; ninfluence < num_cal_influences; ++ninfluence)
			{
				auto& cal_influence = cal_vtx.vectorInfluence[ninfluence];
				assert(cal_influence.boneId < MAX_SUPPORTED_BONES);
				assert(cal_influence.boneId >= 0);
				skin_vtx.bone_ids[ninfluence] = (uint8_t)(cal_influence.boneId);
				// Map 0..1 to 0..255 as uint8_t
				skin_vtx.weights[ninfluence] = (uint8_t)(cal_influence.weight * 255.0f);
			}

			// Fill the rest (from my potencial 4 bones) with zeros
			for (size_t ninfluence = num_cal_influences; ninfluence < max_engine_influences; ++ninfluence)
			{
				skin_vtx.bone_ids[ninfluence] = 0;
				skin_vtx.weights[ninfluence] = 0;
			}

			// Tangents from cal, copy them to our vertex
			auto& cal_tangent = cal_tangents[vid];
			VEC3 mcv_tangent = Cal2DX(cal_tangent.tangent);
			skin_vtx.tangent = VEC4(mcv_tangent.x, mcv_tangent.y, mcv_tangent.z, cal_tangent.crossFactor);

			// Save the vertex in the linear buffer
			mds_vtxs.write(skin_vtx);
		}

		// Cal tiene 2 materiales, tienes 2 submeshes: 1er de 200 vtx y el 2nd de 100 vtx
		// 0...199   0..99
		// El engine tiene 300 vertices (estan todos en la misma malla)
		// Los indices en el engine
		// 0..199    200..299

		int num_faces = cal_sm->getFaceCount();
		const auto& cal_faces = cal_sm->getVectorFace();
		for (auto& cal_face : cal_faces)
		{
			for (int i = 0; i < 3; ++i)
			{
				uint32_t cal_vertex_idx = cal_face.vertexId[i];
				uint32_t engine_vertex_id = cal_vertex_idx + acc_vtxs;

				// Use 16 bits or 32 bits per index?
				if (total_vtxs > 65535)
				{
					mds_idxs.write(engine_vertex_id);
				}
				else
				{
					uint16_t vertex_id_16bits = (uint16_t)engine_vertex_id;
					mds_idxs.write(vertex_id_16bits);   // 2bytes per index
				}
			}
		}

		// After the first sum_mesh, we have 200 vertices
		acc_vtxs += num_vids;
	}

	// MeshIO
	TMeshIO mesh_io = {};
	TMeshIO::THeader& header = mesh_io.header;
	header.header = TMeshIO::magic_header;
	header.version_number = TMeshIO::THeader::current_version;
	header.end_of_header = TMeshIO::magic_end_of_header;
	header.bytes_per_index = total_vtxs > 65535 ? sizeof(uint32_t) : sizeof(uint16_t);
	header.bytes_per_vertex = sizeof(TSkinVertex);
	header.num_indices = total_faces * 3;
	header.num_primitives = total_faces;
	header.num_vertices = total_vtxs;
	header.primitive_type = D3D_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	strcpy(header.vertex_type_name, "PosNUvTanSkin");

	mesh_io.indices = mds_idxs.buffer;
	mesh_io.vertices = mds_vtxs.buffer;
	mesh_io.mesh_groups = groups;

	// Do the real save
	CFileDataSaver fds(out_mesh_name.c_str());
	mesh_io.write(fds);
}

// --------------------- 
bool CGameCoreSkeleton::create(const std::string& res_name) {

  json jData = loadJson(res_name);

  std::string name = jData["name"];
  root_path = "data/skeletons/" + name + "/";

  // Configure the Up axis
  CalLoader::setLoadingMode(LOADER_ROTATE_X_AXIS | LOADER_INVERT_V_COORD | LOADER_FLIP_WINDING);

  std::string csf = root_path + name + ".csf";
  {
    PROFILE_FUNCTION("Skeleton");
    bool is_ok = loadCoreSkeleton(csf);
    if (!is_ok)
      return false;
  }

  auto& anims = jData["anims"];
  for (auto it = anims.begin(); it != anims.end(); ++it) {
    assert(it->is_object());
    auto& anim = *it;
    std::string anim_name = anim["name"];
    PROFILE_FUNCTION(anim_name);
    std::string caf = root_path + anim_name + ".caf";
    int anim_id = loadCoreAnimation(caf, anim_name);
    if (anim_id < 0) {
      fatal("Failed to load animation %s in model %s\n", anim_name.c_str(), name.c_str());
      return false;
    }

	// load callbacks

	auto coreAnim = getCoreAnimation(anim_id);
	coreAnim->getCallbackList().clear();

	auto& callbacks = anim["callbacks"];
	for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
	{
		std::string cb_name = it.value();
		CCallbackFactory cb_factory = CallbackRegistry.get(cb_name);
		CalAnimationCallback* cb = cb_factory();
		coreAnim->registerCallback(cb, 0);
	}

	auto& events = anim["events"];
	for (int i = 0; i < events.size(); ++i)
	{
		const json& item = events[i];
		CCallbackFactory cb_factory = CallbackRegistry.get("event");
		CalAnimationCallback* cb = cb_factory();
		EventCallback* event_cb = (EventCallback*)cb;
		event_cb->parse(item);
		coreAnim->registerCallback(cb, 0);
	}

	// load params
	TParams p;
	p.blendIn = anim.value("blendIn", -1.f);
	p.blendOut = anim.value("blendOut", -1.f);
	p.animTime = anim.value("time", -1.f);

	// get later by ID
	anim_params.push_back(p);
  }

  auto& meshes = jData["meshes"];
  for (auto it = meshes.begin(); it != meshes.end(); ++it)
  {
    std::string mesh_name = it.value();
    std::string cmf = root_path + mesh_name + ".cmf";
    if (fileExists(cmf.c_str()))
    {
      int mesh_id = loadCoreMesh(cmf);
      if (mesh_id < 0)
      {
        fatal("Failed to load mesh %s in model %s\n", mesh_name.c_str(), name.c_str());
        return false;
      }

      // Name to save as output
      std::string engine_mesh = root_path + mesh_name + ".mesh";

      convertCalMeshToEngineMesh(engine_mesh, mesh_id);
      
      // Delete the file from the file system
      _unlink(cmf.c_str());
    }

  }

  float custom_scale = jData.value("scale", 1.0f);
  scale(custom_scale);

  // Array of bone ids to debug (auto conversion from array of json to array of ints)
  if (jData["bone_ids_to_debug"].is_array())
    bone_ids_to_debug = jData["bone_ids_to_debug"].get< std::vector< int > >();

  // Find bones without children to compute the aabb
  bone_ids_for_aabb.clear();
  auto& bones = this->getCoreSkeleton()->getVectorCoreBone();
  int bone_id = 0;
  for (auto bone : bones)
  {
    bool has_children = !bone->getListChildId().empty();
    if (!has_children)
      bone_ids_for_aabb.push_back(bone_id);
    ++bone_id;
  }
  if (jData.count("aabb_extra_factor"))
    aabb_extra_factor = loadVEC3(jData, "aabb_extra_factor");

  // Read shared lookat correction set of bones
  if (jData.count("lookat_corrections")) {

	bool loadAll = false;

	if (jData["lookat_corrections"].is_boolean()) {
	  loadAll = jData.value("lookat_corrections", loadAll);
	}

	if (loadAll) {
	  auto bones = getCoreSkeleton()->getVectorCoreBone();
	  for (int i = 0; i < bones.size(); ++i) {
	  	TBoneCorrection c;
	  	c.bone_name = bones[i]->getName();
	  	c.amount = 0.f;
	  	c.local_axis_to_correct = VEC3::Right;
	  	c.bone_id = getCoreSkeleton()->getCoreBoneId(c.bone_name);
	  	lookat_corrections.push_back(c);
	  }
	}
	else {
	  auto& jcorrs = jData["lookat_corrections"];
	  assert(jcorrs.is_array());
	  for (int i = 0; i < jcorrs.size(); ++i) {
	  	TBoneCorrection c;
	  	c.load(jcorrs[i]);
	  	// Resolve the bone_name to bone_id here
	  	c.bone_id = getCoreSkeleton()->getCoreBoneId(c.bone_name);
	  	lookat_corrections.push_back(c);
	  }
	}
  }

  return true;
}

// ------------------------------------------------------------
static void showCoreBoneRecursive(CalCoreSkeleton* core_skel, int bone_id)
{
	assert(core_skel);
	if (bone_id == -1)
		return;
	CalCoreBone* cb = core_skel->getCoreBone(bone_id);
	char buf[128];
	sprintf(buf, "%s [Id:%d]", cb->getName().c_str(), bone_id);
	//ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Appearing);
	if (ImGui::TreeNode(buf))
	{
		auto children = cb->getListChildId();
		for (auto it : children)
		{
			showCoreBoneRecursive(core_skel, it);
		}
		ImGui::TreePop();
	}
}

CGameCoreSkeleton::TParams CGameCoreSkeleton::getAnimationUserParams(int anim_id)
{
	if (anim_id < 0) {
		fatal("Failed to get animation params from anim id %d in model %s\n", anim_id, name.c_str());
		return TParams();
	}

	return anim_params[anim_id];
}

bool CGameCoreSkeleton::renderInMenu() const {

  CGameCoreSkeleton* ncthis = (CGameCoreSkeleton*)this;
  auto* core_skel = ncthis->getCoreSkeleton();

  if (ImGui::TreeNode("Animations..."))
  {
	  for (int i = 0; i < getCoreAnimationCount(); ++i)
	  {
		  const CalCoreAnimation* anim = getCoreAnimation(i);
		  ImGui::Text("Name: %s", anim->getName().c_str());
		  ImGui::Text("Num Cb: %d", anim->m_listCallbacks.size());
	  }
	  
	  ImGui::TreePop();
  }

  if (ImGui::TreeNode("Hierarchy Bones..."))
  {
    auto& core_bones = core_skel->getVectorRootCoreBoneId();
    for (auto& bone_id : core_bones)
      showCoreBoneRecursive(core_skel, bone_id);
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("All Bones"))
  {
    auto& core_bones = core_skel->getVectorCoreBone();
	ImGui::Text("Num Bones: %d", core_bones.size());
    for (int bone_id = 0; bone_id < core_bones.size(); ++bone_id)
    {
      CalCoreBone* cb = core_skel->getCoreBone(bone_id);
      if (ImGui::TreeNode(cb->getName().c_str()))
      {
        ImGui::LabelText("ID", "%d", bone_id);
        if (ImGui::SmallButton("Show Axis"))
          ncthis->bone_ids_to_debug.push_back(bone_id);
        ImGui::TreePop();
      }
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("AABB Bones"))
  {
    ImGui::DragFloat3("AABB Factor", &ncthis->aabb_extra_factor.x, 0.02f, 1.0f, 2.0f);
    for (auto bone_id : bone_ids_for_aabb)
    {
      CalCoreBone* cb = core_skel->getCoreBone(bone_id);
      ImGui::Text(cb->getName().c_str());
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("LookAt Corrections...")) {
	  int i = 0;
    for (auto& it : ncthis->lookat_corrections) {
      ImGui::PushID(&it);
      it.debugInMenu();
	  if (ImGui::Button("Erase")) {
		ncthis->lookat_corrections.erase(ncthis->lookat_corrections.begin() + i);
		ImGui::PopID();
		break;
	  }
      ImGui::PopID();
	  ++i;
    }
    ImGui::TreePop();
  }

  return false;
}
