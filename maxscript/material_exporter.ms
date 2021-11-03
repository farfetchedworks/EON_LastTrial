struct TMaterialExporter (
	fs = TJsonFormatter(),
	scene_exporter,
	output_path,
	isXref,
	disable_backculling = false,
	material_filenames = #(),
	
	fn quoteFile txt = (
		local file = substituteString txt "\\" "/"
		return "\"" + file + "\""
	),
	
	-- Other formats, please look at "C:/Code/engine/tools/"
	-- out_format = BC1_UNORM
	fn convertTexture in_file out_file out_format folder = (
		
		-- Create folder if does not exist
		local full_folder_path = scene_exporter.bin_path + folder
		if not doesDirectoryExist full_folder_path then(
			makeDir full_folder_path
		)
		
		-- Convert a non-dds file to dds
		local tools_path = scene_exporter.bin_path + "../tools/"
		
		-- Quote the conversor jus in the case tools_path contains spaces
		local conversor = tools_path + "texconv.exe"
		local cmd = quoteFile conversor
		cmd = cmd + " -f " + out_format		-- format
		cmd = cmd + " -pow2"
		local opath = getFilenamePath out_file
		cmd = cmd + " " + (quoteFile in_file)
		
		-- Quoting everything so the doscommand does not complain
		cmd = quoteFile cmd 
		
		-- The conversion leaves the converted filename .dds in the same folder as the input file
		format "%\n" cmd
		doscommand cmd
		
		-- Copy the resulting file to our destination
		local tmp_file = (getFilenamePath in_file)  + (getFilenameFile in_file) + ".dds"
		
		if not doesFileExist tmp_file then (
			throw ("Failed to convert input texture to dds format\n" +  cmd + "\nExpected file is missing:\n" + tmp_file)
		)

		deletefile out_file

		format "copying % to %\n" tmp_file out_file
		renameFile tmp_file out_file
	),
	
	fn exportMap key_name map default_name skip_compression: = (
		
		if skip_compression == unsupplied then ( skip_compression = false )
		
		-- Check first if we are using a ChannelTexmapToTexmap
		if classof(map) == MultiOutputChannelTexmapToTexmap then(
			
			if key_name == "roughness" then (
				key_name = "combined"
				map = map.source_map.Input_map
			)else (
				return false
			)
		)
		
		if map == undefined or hasProperty map "filename" == false then (
			
			if key_name == "roughness" then (
				key_name = "combined"
				default_name = "data/textures/combined_default.dds"
			)
			
			fs.writeKeyValue key_name default_name
			return true
		)
		
		-- Use max to resolve the full path in the local machine
		local jpg_full_filename = (map.filename as AssetUser).getFullFilePath() -- desktop/batman.jpg
		local base_name = getFilenameFile jpg_full_filename			-- batman
		local out_extension = ".dds"
		local dds_resource = base_name + out_extension
		
		-- don't use scene_name, use level_path so we don't duplicate anything
		local resource_folder = scene_exporter.textures_path + output_path + "/"
		local resource_name = resource_folder + dds_resource
		fs.writeKeyValue key_name resource_name
		
		local dds_full_filename = scene_exporter.bin_path + resource_name
		
		local dds_format = "BC7_UNORM"
		if (skip_compression != undefined and skip_compression == true) then ( dds_format = "R8G8B8A8_UNORM" )
		
		-- Convert the jpg_full_filename into dds_full_filename??
		if not doesFileExist dds_full_filename then (
			local curr_ext = getFilenameType jpg_full_filename
			if curr_ext == out_extension then (
				-- The input file is already a .dds file
				format "Copying dds % to %\n" jpg_full_filename dds_full_filename
				copyFile jpg_full_filename dds_full_filename
			) else (
				format "Converting % to %\n" jpg_full_filename dds_full_filename
				convertTexture jpg_full_filename dds_full_filename dds_format resource_folder
			)
		)
		
		return true
	),
	
	fn exportStdMaterial mat = (
		local ofilename = mat.name + ".mat"
		local resource_name = scene_exporter.materials_path + output_path + "/" + ofilename
		local output_filename = scene_exporter.bin_path + resource_name
		format "Exporting material %\n" output_filename
		fs.begin output_filename
		fs.beginObj()
			exportMap "albedo" mat.diffuseMap "data/textures/white.dds"
			fs.writeComma()
			local normalMapTexture
			if (classof mat.bumpMap == Normal_Bump) then (
				normalMapTexture = mat.bumpMap.Normal
			) else (
				normalMapTexture = mat.bumpMap
			)
			exportMap "normal" normalMapTexture "data/textures/null_normal.dds" skip_compression:true
			fs.writeComma()
			exportMap "roughness" mat.glossinessMap "data/textures/white.dds"
			fs.writeComma()
			exportMap "metallic" mat.specularLevelMap "data/textures/black.dds"
			fs.writeComma()
			exportMap "height" mat.displacementMap "data/textures/black.dds"
			fs.writeComma()
			exportMap "emissive" mat.selfIllumMap "data/textures/black.dds"
			
			if ( findString mat.name "plants" != undefined ) then (
				fs.writeComma()
				fs.writeKeyValue "pipeline" "pbr_gbuffer_two_sided_plants.pipeline"
			)else if (disable_backculling) then (
				fs.writeComma()
				fs.writeKeyValue "pipeline" "pbr_gbuffer_two_sided.pipeline"
			)
			
			--Export custom attributes
			local num_ca = custAttributes.count mat
			for idx_ca = 1 to num_ca do (
				local ca = custAttributes.get mat idx_ca
				ca.exportAsComponent fs undefined
			)
			
		fs.endObj()
		fs.end()
		append material_filenames resource_name
	),
	
	fn exportPBRMaterial mat = (
		local ofilename = mat.name + ".mat"
		local is_decal = isDecal mat
		local subpath = output_path
		if is_decal then(
			subpath = "decals"	
		)
		local resource_name = scene_exporter.materials_path + subpath + "/" + ofilename
		local output_filename = scene_exporter.bin_path + resource_name
		format "Exporting material %\n" output_filename
		fs.begin output_filename
		fs.beginObj()
		
		if is_decal then (
			
			fs.writeKeyValue "pipeline" "decals.pipeline"
			fs.writeComma()
			fs.writeKeyValue "casts_shadows" false
			fs.writeComma()
			fs.writeKeyValue "render_channel" "decals"
			fs.writeComma()	
		) 
		
		exportMap "albedo" mat.base_color_map "data/textures/missing.dds"
		fs.writeComma()
		exportMap "normal" mat.norm_map "data/textures/null_normal.dds" skip_compression:true
		fs.writeComma()
		exportMap "roughness" mat.roughness_map "data/textures/white.dds"
		fs.writeComma()
		exportMap "height" mat.displacement_map "data/textures/black.dds"
		fs.writeComma()
		exportMap "emissive" mat.emit_color_map "data/textures/black.dds"
	
		if ( findString mat.name "plants" != undefined ) then (
				fs.writeComma()
				fs.writeKeyValue "pipeline" "pbr_gbuffer_two_sided_plants.pipeline"
		)else if (disable_backculling) then (
			fs.writeComma()
			fs.writeKeyValue "pipeline" "pbr_gbuffer_two_sided.pipeline"
		)
		
		--Export custom attributes
		local num_ca = custAttributes.count mat
		for idx_ca = 1 to num_ca do (
			local ca = custAttributes.get mat idx_ca
			ca.exportAsComponent fs undefined
		)
		
		-- export material ctes
		local emission_map = mat.emit_color_map
		if emission_map != undefined then(
			local rgb_level = emission_map.output.RGB_Level
			
			if rgb_level != 1.0 then(
				fs.writeComma()
				fs.writeKey "buffers"
				fs.beginObj()
					fs.writeKey "CtesMaterial"
					fs.beginObj()
						fs.writeKeyValue "material_emissive_factor" rgb_level
					fs.endObj()
				fs.endObj()
			)
		)
			
		fs.endObj()
		fs.end()
		append material_filenames resource_name
		
	),
	
	fn exportMultiMaterial multimat = (
		format "exporting multimaterial %" multimat
		local groups = get_faces_by_material scene_exporter.curr_obj

		for idx = 1 to groups.count do (
			local mesh_group = groups[ idx ] 
			if mesh_group == undefined then continue
			local mat_for_this_group = multimat[idx]
			if mat_for_this_group == undefined then (
				-- Fake with a null for this position, the scene_exporter will then assign the default material
				append material_filenames undefined
			) else (
				-- exportStdMaterial multimat[idx]
				if classof mat_for_this_group == StandardMaterial then (
					exportStdMaterial mat_for_this_group
				) else if classof mat_for_this_group == PBRMetalRough then (
					exportPBRMaterial mat_for_this_group
				) else if classof mat_for_this_group == Multimaterial then (
					throw ("I can't export succession of MultiMats" )
				) else if classof mat_for_this_group == XRef_Material then (
					return exportMaterial (mat_for_this_group.GetSrcItem())
				) else (
					throw ("I don't know how to export material of class " + ((classof mat_for_this_group) as string) + " in object " + scene_exporter.curr_obj.name )
					throw "Invalid mat class"
				)
			)
		)
		
	),
	
	fn exportLocalMaterial mat = (
		local ofilename = mat.name + ".mat"
		local resource_name = scene_exporter.materials_path + ofilename
		local output_filename = scene_exporter.bin_path + resource_name
		format "Exporting material %\n" output_filename
		append material_filenames resource_name
	),
	
	
	fn exportMaterial mat = (
		
		output_path = scene_exporter.level_path
		
		-- if not xref, put it in the scene folder
		if(isXref == false) then (
			output_path = scene_exporter.scene_name
		)
		
		if mat != undefined and mat.name == "water" then (
			exportLocalMaterial mat
		)
		else if classof mat == StandardMaterial then (
			exportStdMaterial mat
		) else if classof mat == Multimaterial then (
			exportMultiMaterial mat
		) else if classof mat == PBRMetalRough then (
			exportPBRMaterial mat
		) else if classof mat == XRef_Material then (
			return exportMaterial (mat.GetSrcItem())
		) else if mat == undefined then (
			return undefined
		) else (
			throw ("I don't know how to export material of class " + ((classof mat) as string) + " in object " + scene_exporter.curr_obj.name )
			throw "Invalid mat class"
		)
	)
)
