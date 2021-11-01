utility MyMCVExporter "Exporter"
(
	
	rollout exporter "Exporter"
	(
		local filename
		
		--button 'btn_outputFile' "Output File" toolTip:"Select the output file (select the bin directory for scene exporting)"
		label lab0 "Single exports"
		button btn_mesh "Export Mesh"
		button btn_collision "Export Collision"
		label lab1 "Scene exports"
		button btn_scene "Export Scene"
		button btn_scene_collapsed "Export Scene CCSS"
		button btn_scene_collapsed_fast "Fast Export Scene CCSS"
		\
		label lab2 "Animation"
		button btn_skel_data "Export Skeleton And Meshes"
		button btn_skel_anim "Export Skeletal Animation"
		button btn_rigid_anim "Export Rigid Animation"

		
-- 		-- Obtains the output for the scene
-- 		on btn_outputFile pressed do
-- 		(
-- 			filename = getSavePath \
-- 				caption:"Mesh exporter" \
-- 				filename:(getDir #maxroot + "/") \
-- 			
-- 			filename = filename + "/"
-- 		)
		
		/*
		-- Obtains the output file name
		on btn_outputFile pressed do
		(
			filename = getSaveFileName \
				caption:"Mesh exporter" \
				filename:(getDir #maxroot + "/") \
		)
		*/
		
		-- Exports the current selection mesh to file
		on btn_mesh pressed do
		(
			try (
				if $ == undefined then throw "You must select an object to export"
				gc()
				export_mesh $ undefined undefined
				MessageBox "Single mesh exported OK"
			)
			catch (
				messageBox ("Error Exporting Single Mesh:\n" + getCurrentException())
			)
		)
		
		-- Exports the current selection mesh to file
		on btn_collision pressed do
		(
			try (
				if $ == undefined then throw "You must select an object to export as collision"
				gc()
				export_mesh $ undefined "Pos"
				MessageBox "Single mesh exported OK"
			)
			catch (
				messageBox ("Error Exporting Single Collision Mesh:\n" + getCurrentException())
			)
		)
				
		-- Exports the current selection mesh to file
		on btn_scene pressed do
		(
			try (
				gc()
				local se = TSceneExporter()
				se.exportScene filename false
				MessageBox "Scene exported OK"
			)
			catch (
				messageBox ("Error Exporting Scene:\n" + getCurrentException())
			)
		)
		
		-- Exports the current selection mesh to file
		on btn_scene_collapsed pressed do
		(
			try (
				gc()
				local se = TSceneExporter()
				se.exportScene filename true
				MessageBox "Scene with CCSS exported OK (meshes included)"
			)
			catch (
				messageBox ("Error Exporting Scene with collapsed collisions:\n" + getCurrentException())
			)
		)
	
		on btn_scene_collapsed_fast pressed do
		(
			try (
				gc()
				local se = TSceneExporter()
				se.fast_export = true
				se.exportScene filename true
				MessageBox "Scene with CCSS exported OK (no meshes included)"
			)
			catch (
				messageBox ("Error Exporting Scene with collapsed collisions:\n" + getCurrentException())
			)
		)
		
		-- Exports the current selection mesh to file
		on btn_skel_data pressed do
		(
			try (
				local se = TSkeletonsExporter()
				se.exportSkelAndMeshes()
				MessageBox "Skeleton and Meshes exported OK"
			)
			catch (
				messageBox ("Error Exporting Skeleton:\n" + getCurrentException())
			)
		)
		
		-- Exports the current selection mesh to file
		on btn_skel_anim pressed do
		(
			try (
				local se = TSkeletonsExporter()
				se.exportAnim()
				MessageBox "Skeleton Animation exported OK"
			)
			catch (
				messageBox ("Error Exporting Skeletal Animation:\n" + getCurrentException())
			)
		)
		
		-- Exports a rigid animation from the selected meshes
		on btn_rigid_anim pressed do
		(
			try (
				
				local anim_name = getFilenameFile  maxfilename
				if anim_name == "" then throw "Please, save the .max file to some name before exporting the animation"
							
				local ex = TAnimationExporter()
				objs = getCurrentSelection()
				for o in objs do
				(
					ex.exportObj o
				)
				
				ex.writeAnimation anim_name
				MessageBox "Rigid Animation exported OK"
			)
			catch (
				messageBox ("Error Exporting Rigid Animation:\n" + getCurrentException())
			)
		)
	)
	
	-- ...
	on MyMCVExporter open do
	(
		--- read from an .ini the current directory path...
		addRollout exporter
	) 
	on MyMCVExporter close do
	(
		removeRollout exporter
	) 
	
)
