clearListener()

-- Plugin: https://www.worldviz.com/virtual-reality-software-downloads

fn isBone obj = (
	return (classof obj == Biped_Object) or  (classof obj == Dummy) or (classof obj == BoneGeometry)
)

fn isRootBone obj = (
	return isBone obj and (obj.parent == undefined or (not isBone obj.parent) )
)

fn isBoneCandidate obj = ( 
	-- Discard black bones
	return isBone obj and obj.wireColor != Black
)

struct  TSkeletonsExporter (
	skel_name = "unknown",			-- maw
	skel_root_path,					-- "C:/../bin/data/skeletons/maw/,
	skel_file, 						-- "C:/../bin/data/skeletons/maw/maw.csf"
	
	-- Find a bone candidate to be the root, and choose his name as name of the output file
	fn findRootBones = (
		local obj_bones = #()
		for obj in $* do (
			if isRootBone obj then (
				format "% is a root bone\n" obj.name
				append obj_bones obj
			)
		)
		return obj_bones
	),
	
	fn setSkelName new_name = (
		skel_name = new_name

		iniFileName = getMaxiniFile()
		localBinPath = getINISetting iniFileName "ExportDirsMCV" "binDir"

		skel_root_path = localBinPath + "data/skeletons/" + skel_name + "/"
		makedir skel_root_path 
		skel_file = skel_root_path + skel_name + ".csf"
		format "Skel file %\n" skel_file
	),
	
	-- Find a bone candidate to be the root, and choose his name as name of the output file
	fn findRootBone = (
		local roots = findRootBones()
		-- A single candidate... return it
		if roots.count == 0 then throw "No root bones detected in the scene!"
		if roots.count > 1 then throw "More than one root bone detected in the scene!"
		local root = roots[1]
		setSkelName root.name
		return root
	),
	
	-- Find in the scene the bones which will be exported as part of the animation
	-- Change the color to black to discard the bones
	fn findBoneCandidates = (
		local objs = #()
		for obj in $* do (
			if isBoneCandidate obj then (
				--format "% is a bone for the animation\n" obj.name
				append objs obj
			)
		)
		return objs
	),	
	
	-- Returns an array of meshes which have the skin modifier and will be exported as .cmf
	fn findMeshCandidates = (
		local objs = #()
		for obj in $* do (
			if (classof obj == PolyMeshObject) or (classof obj == Editable_Mesh) or (classof obj == Editable_Poly) then (
				if obj.modifiers[#Skin] != undefined then (
					format "% is a skinned mesh \n" obj.name
					append objs obj
				)
			)
		)
		return objs
	),
	
	fn exportSkelAndMeshes = (
		if ExportCalSkel == undefined then throw "Please, install the cal3d export plugin"
		local root = findRootBone()
		local bones_to_export = findBoneCandidates()
		print bones_to_export
		
		-- Ensure we are in figure mode when exporting skeleton and meshes!!!!
		-- We can only do this if the skeleton is a biped 
		local was_in_figure_mode = false
		if( classof root.controller != prs ) then (
			was_in_figure_mode = root.controller.figureMode
			if biped.numLayers root.controller != 0 then throw ("Biped " + root.name + " has active layers. Please collapse them before exporting the file so we can place the skieleton in figure Mode" )
			root.controller.figureMode = true
		)
		
		-- Do the real export
		format "Exporting cal csf to %\n" skel_file
		local rc = ExportCalSkel skel_file bones_to_export off
		if rc != 0 then throw ("ExportCalSkel: Failed to export cal3d skel. rc:" + ( rc as string ))
			
		local meshes = findMeshCandidates()
		for obj in meshes do (
			local mesh_file = skel_root_path + obj.name + ".cmf"
			if obj.ishidden then continue
			format "Exporting cal mesh to %\n" mesh_file
			-- 0.01 threshold, no LOD, no springs
			local rc = ExportCalMesh mesh_file skel_file obj 4 0.01 false false
			if rc != 0 then throw ("ExportCalMesh: Failed to export cal3d mesh from object " + obj.name + " rc:" + ( rc as string ) )
		)
			
		-- Restaurar el modo pose si es q estaba en modo pose inicialmente
		if( classof root.controller != prs and was_in_figure_mode == false ) then root.controller.figureMode = false

	),
	
	fn getAnimationMaxTime objs = (
		local max_time = 0
		for obj in objs do (
			
			local controller = undefined
			
			try
			(
				controller = obj.position.controller
			)
			catch
			(
				controller = obj.controller
			)	
			
			-- no valid controller at all
			if controller == undefined then continue
			
			local nkeys = numkeys controller
			if nkeys == -1 or nkeys == 0 then continue
			
			local last_time = getKeyTime controller nkeys
			if last_time > max_time then max_time = last_time
		)
		return max_time
	),
	
	
	fn exportAnim = (
		local root = findRootBone()
		local bones_to_export = findBoneCandidates()
		
		-- Export anims. 
		-- Take the name of the output file from the .max filename
		local anim_name = getFilenameFile  maxfilename
		if anim_name == "" then throw "Please, save the .max file to some name before exporting the animation"
		local animation_file = skel_root_path + anim_name + ".caf"
		
		-- frames to integer-frames
		-- From 60f we need to get the 60 as integer. 4800 is defined by Max documentation
		local min_time = 0f
		local max_time = getAnimationMaxTime bones_to_export
		
		local begin_time = min_time as integer * frameRate / 4800  
		local end_time = max_time as integer  * frameRate / 4800
		format "Exporting anim % from % to %\n" animation_file begin_time end_time
		local rc = ExportCalAnim animation_file skel_file bones_to_export begin_time end_time 0 frameRate
		if rc != 0 then throw ("ExportCalAnim: Failed to export cal3d anim. rc:" + (rc as string) )
	)
)

--se = TSkeletonsExporter()
--se.exportAnim()

