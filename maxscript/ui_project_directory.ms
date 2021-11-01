utility UtilProjectDirectory "Project Directory"
(
	
	rollout rProjectDirectory "Project Directory"
	(
		local filename
		
		button 'btn_outputFile' "Select project bin directory" toolTip:"Select bin directory"
		
		-- Obtains the output for the scene
		on btn_outputFile pressed do
		(
			filename = getSavePath \
				caption:"Mesh exporter" \
				filename:(getDir #maxroot + "/") \
			
			if filename != undefined then (
				-- set the bin path and the path for render, collision and prefabs
				filename = filename + "\\"
				renderMeshDir = filename + "data\\meshes\\"
				collisionMeshesDir = filename + "data\\collision_meshes\\"
				prefabsDir = filename + "data\\prefabs\\"
				
				-- set the values in the ini file
				iniFileName = getMaxiniFile()
				setINISetting iniFileName "ExportDirsMCV" "binDir" filename
				setINISetting iniFileName "ExportDirsMCV" "renderMeshesDir" renderMeshDir
				setINISetting iniFileName "ExportDirsMCV" "collisionMeshesDir" collisionMeshesDir
				setINISetting iniFileName "ExportDirsMCV" "prefabsDir" prefabsDir
				
				message = "Project directory successfully updated!"
				
				MessageBox message
			)
			
		)

	)
	
	on UtilProjectDirectory open do
	(
		addRollout rProjectDirectory
	) 
	on UtilProjectDirectory close do
	(
		removeRollout rProjectDirectory
	) 
	
)
