clearListener()

struct TChildrenInfo (
	children = #(),
	physics_shapes = #(),
	parent = undefined,
	hasColliders = false,
	on create do (
		for child in parent.children do (
			if isKeepCollider child then (
				append physics_shapes child
			)
			else if isCollider child then (
				hasColliders = true
				append physics_shapes child
			)
			else if isTrigger child then (
				append physics_shapes child
			)
			else (
				append children child 
			)
		)
	)
)

struct TSceneExporter (
	bin_path, 			-- E:/Code/mcv/bin/
	scene_name,
	level_path = "common",
	xref_scene_name,
	output_data_path,
	output_filename,
	scenes_path = "data/scenes/",	
	meshes_path = "data/meshes/",
	textures_path = "data/textures/",
	materials_path = "data/materials/",
	collisions_path = "data/collision_meshes/",
	objs_exported,
	xrefs_array,	    -- array containing exported xref prefabs
	checking_xrefs,
	collapsed_colliders,
	fast_export = false,
	default_cull_distance = 150,
	
	-- Context
	curr_obj,
	
	fn exportCompName obj_name fs = (
		fs.writeKeyValue "name" obj_name
	),
	
	fn exportCompTransform is_xref_prefab fs = (
		fs.writeKey "transform"

		local max_transform
		if is_xref_prefab then (
			max_transform = Matrix3 [1, 0, 0] [0, 1, 0] [0, 0, 1] [0, 0, 0]
		) else (
			max_transform = curr_obj.transform
		)
		
		local is_decal = isDecal curr_obj.material
		local max_to_mcv = RotateXMatrix -90
		local mcv_to_max = RotateXMatrix 90
		local mcv_transform = mcv_to_max * max_transform * max_to_mcv
		
		local mcv_pos_meters = inMeters( mcv_transform.position )
		local mcv_scale = mcv_transform.scale
		local mcv_rot = mcv_transform.rotation
		fs.beginObj()
			fs.writeKeyValue "pos" mcv_pos_meters
			fs.writeComma()

		-- If object is spotlight, we need target position for lookat instead
		if (classof curr_obj == targetSpot) then (
			local target_trans = mcv_to_max * curr_obj.Target.transform * max_to_mcv
			fs.writeKeyValue "lookat" (inMeters target_trans.position)
		) else (
			if is_decal and classof(curr_obj) == Box then(
				local scale_decal = inMeters(curr_obj.width) as string + " " + inMeters(curr_obj.height) as string + " " + inMeters(curr_obj.length) as string
				fs.writeKeyValue "scale" scale_decal
			)else(
				fs.writeKeyValue "scale" mcv_scale
			)
			fs.writeComma()
			fs.writeKeyValue "rot" mcv_rot
		)

		fs.endObj()
	),
	
	fn exportMaterialResource = (
		local mat_exporter = TMaterialExporter()
		mat_exporter.scene_exporter = this
		mat_exporter.disable_backculling = not curr_obj.backFaceCull
		mat_exporter.isXref = checking_xrefs == false and classof(curr_obj) == XRefObject
		mat_exporter.exportMaterial curr_obj.mat
		return mat_exporter.material_filenames
	),
	
	fn exportDrawCall mesh_group mesh_group_idx fs is_xref = (
		local opath = level_path
		if is_xref == false then (opath = scene_name)
		local mat_resources = exportMaterialResource()
		local mesh_resource = meshes_path + opath + "/" + curr_obj.name + ".mesh"
		local mat_resource = mat_resources[mesh_group_idx]
		if mat_resource == undefined then mat_resource = materials_path + "missing.mat"
		local is_decal = findString mat_resource "/decal" != undefined
		if is_decal then (
			mesh_resource = "data/meshes/cube_1m.mesh"
		)
		fs.beginObj()
			fs.writeKeyValue "mesh" mesh_resource
				fs.writeComma()
			fs.writeKeyValue "mat" mat_resource
				fs.writeComma()
			fs.writeKeyValue "mesh_group" (mesh_group_idx - 1)
		fs.endObj()
		
		return is_decal
	),
	
	fn exportCompRender fs = (
		
		local isXref = checking_xrefs == false and classof(curr_obj) == XRefObject
		
		-- Ensure the obj is an editable mesh
		local curr_mesh = curr_obj
		if classof curr_mesh != Editable_Mesh then (
			-- Do a copy, and convert the copy to an EditableMesh
			curr_mesh = copy curr_obj
			convertToMesh curr_mesh
			curr_mesh.name = curr_obj.name
		)
		
		-- Scan materials used by this obj
		local mesh_groups = get_faces_by_material curr_mesh
		
		if curr_mesh.mat != undefined then (
			
			-- There are more material groups but the material is not a multi-material
			if mesh_groups.count > 1 and classof curr_mesh.mat != MultiMaterial then (
				format "Hey, the mesh has % groups, but the material % is not a multimaterial\n" mesh_groups.count curr_mesh.mat.name 
				if curr_mesh != curr_obj then (
					format "Setting all faces to use the first material id\n"
					for face_id = 1 to getNumFaces curr_mesh do (
						setfacematid curr_mesh face_id 1
					)
					mesh_groups = get_faces_by_material curr_mesh
				)
			)
			
		)

		-- Add aabb for culling
		fs.writeKeyValue "abs_aabb" "true"

		fs.writeComma()
		
		fs.writeKey "render"

		local hasDecal = false
		
		fs.beginArray()

			local group_idx = 1
			for mesh_group in mesh_groups do (
				if mesh_group == undefined then continue
				if group_idx > 1 then fs.writeComma()
				hasDecal = hasDecal or exportDrawCall mesh_group group_idx fs isXref
				group_idx = group_idx + 1
			)
		
		fs.endArray()
			
		if hasDecal then (
			fs.writeComma()
			fs.writeKeyValue "gpu_instancing" "true"
		)
		
		local opath = level_path
		local object_name = curr_obj.name
		
		if isXref == false then (
			opath = scene_name
		)else (
			object_name = curr_obj.objectName
		)
		
			
		-- Do the real mesh geometry export once
		-- if not in fast export mode
		if(not fast_export) then (
			local mesh_resource = meshes_path + opath +  "/" + curr_obj.name + ".mesh"
			local vertex_type = undefined
			if not hasDecal then (export_mesh curr_mesh (bin_path + mesh_resource) vertex_type)
		)
		
		-- Delete the temporal copy created in case the curr_obj was not a mesh
		if curr_mesh != curr_obj then delete curr_mesh
	),
	
	fn exportCompHierarchy parent_obj fs = (
		fs.writeKey "hierarchy"
		fs.beginObj()
		fs.writeKeyValue "parent" parent_obj.name
		fs.endObj()
	),
	
	fn exportCompParent children_info fs = (
		fs.writeKey "parent_of"
		fs.beginObj()
		fs.endObj()
		--fs.beginArray()

		--local nobjs = 0
		--for child in children_info.children do (
			--fs.writeName child.name
			--nobjs = nobjs + 1

			--if nobjs < children_info.children.count then fs.writeComma()
		--)

		--fs.endArray()
	),

	fn exportShape objshape fs = (
		fs.beginObj()
		local obj = objshape
		if (classof(objshape) == XRefObject) then (
			obj = obj.actualBaseObject
		)
		if classof obj == Sphere then (
			fs.writeKeyValue "shape" "sphere"
			fs.writeComma()
			fs.writeKeyValue "radius" ( inMeters obj.radius )
			
		) else if classof obj == Box then (
			fs.writeKeyValue "shape" "box"
			fs.writeComma()
			-- Check the sizes x,y,z order is correct!!
			local half_sizes = [obj.width, obj.height, obj.length] * 0.5
			local half_sizes_mtrs = inMeters half_sizes
			fs.writeKeyValue "half_size" ( half_sizes_mtrs )
			
			-- Only add the offset in Y, the other ones are correct
			half_sizes_mtrs[1] = 0.0
			half_sizes_mtrs[3] = 0.0
			fs.writeComma()
			fs.writeKey "offset"
			fs.beginObj()
				fs.writeKeyValue "pos" (half_sizes_mtrs)
			fs.endObj()
			
		) else if classof obj == Capsule then (
			fs.writeKeyValue "shape" "capsule"
			fs.writeComma()
			fs.writeKeyValue "radius" ( inMeters obj.radius )
			fs.writeComma()
			fs.writeKeyValue "height" ( inMeters obj.height )
			
		) else (
			
			local as_convex_mesh_str = getUserProp curr_obj "convex" 
			local as_convex_mesh = (as_convex_mesh_str == 1) or (as_convex_mesh_str == "true")
			format "as_convex_mesh % str --%--\n" as_convex_mesh as_convex_mesh_str
			if as_convex_mesh == true then (
				fs.writeKeyValue "shape" "convex"
			) else (
				fs.writeKeyValue "shape" "trimesh"
			)
			
			local cmesh_resource = collisions_path + scene_name + "/" + objshape.name + ".cmesh"
			fs.writeComma()
			fs.writeKeyValue "collision_mesh" cmesh_resource

			-- Do the real export to cmesh
			if(not fast_export) then (export_mesh objshape (bin_path + cmesh_resource) "Pos")
		)

		if isTrigger objshape then (
			fs.writeComma()
			fs.writeKeyValue "trigger" true
			fs.writeComma()
			fs.writeKeyValue "group" "trigger"
		)

		-- compute offset wrt my parent
		-- export as matrix and attribute 'offset'
		
		if (getUserProp curr_obj "group") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "group" (getUserProp curr_obj "group")
		)
    
		if (getUserProp curr_obj "mask") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "mask" (getUserProp curr_obj "mask")
		)
			
		fs.endObj()
	),
	
	fn exportCompCollider children_info fs = (
		fs.writeKey "collider"

		fs.beginObj()
			fs.writeKey "shapes"
			fs.beginArray()
			local nshapes = 0
			for obj in children_info.physics_shapes do (
				if nshapes > 0 then fs.writeComma()
				exportShape obj fs
				nshapes = nshapes + 1
			)
			fs.endArray()
			if getUserProp curr_obj "dynamic" != undefined then (
				fs.writeComma()
				fs.writeKeyValue "dynamic" true
			)
		
			if getUserProp curr_obj "kinematic" != undefined then (
				fs.writeComma()
				fs.writeKeyValue "kinematic" true
			)
		fs.endObj()
	),

	fn exportPointLight obj fs = (
		fs.writeKey "light_point"
		fs.beginObj()
		fs.writeKeyValue "enabled" obj.enabled
		fs.writeComma()
		fs.writeKeyValue "color" obj.rgb
		fs.writeComma()
		fs.writeKeyValue "radius" (inMeters (obj.decayRadius) )
		fs.writeComma()
		fs.writeKeyValue "intensity" obj.multiplier
		
		if (getUserProp obj "irradiance_intensity") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "irradiance_intensity" (getUserProp curr_obj "irradiance_intensity")
		)
		
		fs.endObj()
	),

	fn exportSpotLight obj fs = (
		-- activate always culling for light, change this if light doesnt cast shadows in the future
		fs.writeKeyValue "culling" "true"
		fs.writeComma()

		fs.writeKey "light_spot"
		fs.beginObj()
		fs.writeKeyValue "enabled" obj.enabled
		fs.writeComma()
		fs.writeKeyValue "color" obj.rgb
		fs.writeComma()
		fs.writeKeyValue "intensity" obj.multiplier
		fs.writeComma()
		fs.writeKeyValue "z_min" 1
		
		local z_far = (inMeters (distance obj obj.Target))
		if (z_far < 1) then ( z_far = 1.1 )
		
		local pattern_info = getUserProp obj "pattern"
		local pattern_tex = "data/textures/pattern_vidriera_color.dds"
		local is_ortho = true
			
		if (pattern_info != undefined) then (
			fs.writeComma()
			fs.writeKeyValue "use_pattern" true
			fs.writeComma()
			
			if pattern_info == "round" then (
				pattern_tex = "data/textures/pattern_vidriera_redonda_color.dds"
				is_ortho = false
			)
			
			fs.writeKeyValue "pattern" pattern_tex
			fs.writeComma()
			fs.writeKeyValue "ortho" is_ortho
			fs.writeComma()
			fs.writeKeyValue "ortho_centered" true
			fs.writeComma()
			fs.writeKeyValue "ortho_width" 6.5
			fs.writeComma()
			fs.writeKeyValue "ortho_height" 5
			fs.writeComma()
			fs.writeKeyValue "z_max" (z_far)
		)else (
		
			fs.writeComma()
			fs.writeKeyValue "fov" obj.falloff
			fs.writeComma()
			fs.writeKeyValue "z_max" z_far
		)
		
		fs.writeComma()
		fs.writeKeyValue "casts_shadows" true
		fs.writeComma()
		fs.writeKeyValue "shadows_fmt" "R32_TYPELESS"
			
		if (getUserProp obj "shadows_res") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "shadows_resolution" (getUserProp curr_obj "shadows_res")
		)
		
		if (getUserProp obj "godrays") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "cast_godrays" (getUserProp curr_obj "godrays")
		)
		
		if (getUserProp obj "godrays_intensity") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "godrays_intensity" (getUserProp curr_obj "godrays_intensity")
		)
		
		if (getUserProp obj "irradiance_intensity") != undefined then (
			fs.writeComma()
			fs.writeKeyValue "irradiance_intensity" (getUserProp curr_obj "irradiance_intensity")
		)
			
		fs.endObj()
	),
	
	fn exportObj obj parent_obj obj_name check_xref fs = (),

	fn exportRefScene ref_obj = (

		local tmp_name = getFilenameFile ref_obj.filename
		local appended = appendIfUnique xrefs_array tmp_name

		if appended == false then return()

		xref_scene_name = tmp_name

		iniFileName = getMaxiniFile()
		bin_path = getINISetting iniFileName "ExportDirsMCV" "binDir"

		output_data_path = bin_path + "data/"

		if xref_scene_name == "" then xref_scene_name = "unnamed"
		output_filename = output_data_path + "prefabs/" + level_path + "/" + xref_scene_name + ".json"
		local fs = TJsonFormatter()
		fs.begin output_filename
		fs.beginArray()

		exportObj ref_obj undefined ref_obj.srcItemName false fs

		fs.endArray()
		fs.end()
	),
	
	fn exportGPUInstanced fs obj = (

		local dist_threshold = default_cull_distance
		
		if( getUserProp obj "cull_dist" != undefined ) then (
			dist_threshold = getUserProp obj "cull_dist"
		)
		
		local replacement_prefab = undefined
		
		if( getUserProp obj "lod" != undefined ) then (
			replacement_prefab = getUserProp obj "lod"
		)
		
		fs.writeComma()
		fs.writeKey "gpu_instancing"
		fs.beginObj()
			fs.writeKeyValue "threshold" dist_threshold
			if replacement_prefab != undefined then(
				fs.writeComma()
				fs.writeKeyValue "replacement_prefab" replacement_prefab	
			)
		fs.endObj()

	),

	fn exportObj obj parent_obj obj_name check_xref fs = (

		local children_info = TChildrenInfo parent:obj

		checking_xrefs = check_xref
		curr_obj = obj

		fs.beginObj()
		fs.writeKey "entity"
		fs.beginObj()
			exportCompName obj_name fs
				fs.writeComma()
			exportCompTransform (not check_xref) fs

			-- Export xref
			if (check_xref == true and classof(obj) == XRefObject) then (

				local prefab_name = getFilenameFile obj.srcFileName
				local src = "data/prefabs/" + level_path + "/" + prefab_name + ".json"
				fs.writeComma()
				fs.writeKeyValue "prefab" src
				fs.endObj()
				fs.endObj()

				exportRefScene obj

				return()
			)

			-- Check is there is a prefab or component to export 
			-- to avoid exporting a useless CompRender
			local noCompRender = false
			local no_gpu_instancing = false
			local num_ca = custAttributes.count obj
			--local is_dummy = getUserProp obj "dummy"
			for idx_ca = 1 to num_ca do (
				local ca = custAttributes.get obj idx_ca
				if (ca.name == "prefabData" or 
					ca.name == "deathAreaData" or
					ca.name == "TriggerAreaData" or
					ca.name == "IrradianceVolumeData") then (
					noCompRender = true
				) 
				
				if (ca.name == "classAnimationLauncherData" or 
					ca.name == "classEmissiveMorseData") then (
					no_gpu_instancing = true
				) 
			)

			if (classof obj == Dummy) then (
				noCompRender = true
			)

			--format "class type %\n" classof(obj)
			if (classof(obj) == Omnilight) then (
				noCompRender = true
				fs.writeComma()
				exportPointLight obj fs
			)

			if (classof(obj) == targetSpot) then (
				noCompRender = true
				fs.writeComma()
				exportSpotLight obj fs
			)

			if noCompRender == false then (
				fs.writeComma()
				exportCompRender(fs)
			)

			local exportColls = (collapsed_colliders == false) or (children_info.hasColliders == false) or (collapsed_colliders == true and obj.name == "collapsed_parent")
			-- format "export colls %\n" exportColls
			if exportColls and children_info.physics_shapes.count > 0 then (
				fs.writeComma()
				exportCompCollider children_info fs
			)
			-- Add gpu_instancing inside prefab 
			else if (check_xref == false and classof(obj) == XRefObject) then (
				
				local skinned = getUserProp obj "skin" 
				
				if (skinned == undefined or skinned == false) and (no_gpu_instancing == false) then(
					exportGPUInstanced fs obj
				)
			)
			
			if parent_obj != undefined then (
				fs.writeComma()
				exportCompHierarchy parent_obj fs
			)
			
			-- Export cust Attributes of this obj...
			for idx_ca = 1 to num_ca do (
				local ca = custAttributes.get obj idx_ca
				ca.exportAsComponent fs obj
			)

			if children_info.children.count > 0 then (
				fs.writeComma()
				exportCompParent children_info fs
			)
			
			-- export navmesh component
			if (obj.name == "collapsed_parent") then (
				fs.writeComma()
				fs.writeKey "navmesh"
				fs.beginObj()
				fs.writeKeyValue "name" ("data/nav_meshes/" + scene_name + ".navmesh")
				fs.endObj()
			)

		fs.endObj()
		fs.endObj()

		-- If obj has children which are not collisions or triggers, export them having me as parent
		local nobjs = 0
		for child in children_info.children do (

			if classof(child) == Skylight or classof(child) == Targetobject then continue

			fs.writeComma()
			exportObj child obj obj.name check_xref fs
			nobjs = nobjs + 1
		)
	),
	
	fn exportCurve obj = (

		iniFileName = getMaxiniFile()
		bin_path = getINISetting iniFileName "ExportDirsMCV" "binDir"

		output_data_path = bin_path + "data/"

		output_filename = output_data_path + "curves/" + obj.name + ".curve"
		local fs = TJsonFormatter()

		fs.begin output_filename
		fs.beginObj()
			fs.writeKeyValue "loop" (isClosed obj 1)
			fs.writeComma()
			fs.writeKey "knots"
			fs.beginArray()

			local max_to_mcv = RotateXMatrix -90
			for idx = 1 to (numknots obj 1) do (
				if idx != 1 then fs.writeComma()
				local vertex = (getKnotPoint obj 1 idx)
				fs.writeValue (inMeters (vertex * max_to_mcv))
			)

			fs.endArray()

		fs.endObj()
		fs.end()

	),

	fn exportScene localBinPath withColCollapsed = (

		if localBinPath == undefined then (
			iniFileName = getMaxiniFile()
			localBinPath = getINISetting iniFileName "ExportDirsMCV" "binDir"		
			--throw("Select the bin folder of the engine")
		)

		collapsed_colliders = withColCollapsed

		if (collapsed_colliders == true) then (
			collapseColliders()
		)

		xrefs_array = #()

		bin_path = localBinPath
		output_data_path = bin_path + "data/"
		scene_name = getFilenameFile maxfilename
		if scene_name == "" then scene_name = "unnamed"
		output_filename = bin_path + scenes_path + scene_name + ".json"

		local fs = TJsonFormatter()
		fs.begin output_filename
		fs.beginArray()
		local nobjs = 0
		for obj in $* do (
			if obj.parent != undefined then continue

			if classof(obj) == Skylight or classof(obj) == Targetobject then continue

			if classof(obj) == line then (
				exportCurve obj
				continue
			)

			if nobjs > 0 then fs.writeComma()
			exportObj obj undefined obj.name true fs
			nobjs = nobjs + 1
		)

		fs.endArray()
		fs.end()
	)
)

--se = TSceneExporter()
--se.exportScene()
--ci = TChildrenInfo parent:$


