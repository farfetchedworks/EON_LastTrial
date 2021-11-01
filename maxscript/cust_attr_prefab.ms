
-- Struct created to get the prefabs list (loops are not allowed in rollouts)
struct TPrefabsInfo (
	prefabsList = #(),
	on create do (
		-- Get prefabs directory
		local iniFileName = getMaxiniFile()
		local prefabsPath = getINISetting iniFileName "ExportDirsMCV" "prefabsDir"
		
		-- Get prefabs names from all files in the directory
		local wildcardString = prefabsPath + "*.json"
		prefabsFiles = getFiles wildcardString
		--format "%\n" prefabsFiles 
		
		if (prefabsFiles.count > 0) then (		
			-- Fill the list with the prefabs files names
			for prefabFile in prefabsFiles do
			(
				local prefabName = getFilenameFile prefabFile
				substituteString prefabName "_" " "
				append prefabsList prefabName	
			)
		) else (
			append prefabsList ""
		)
	)
)


classPrefabData = attributes prefabData attribID: #(0x2d8bf3fb, 0x22b1a736)
(
	
	local prefabs_info = TPrefabsInfo()
	
	parameters main rollout:params
	(
		prefab type:#integer ui:ddlPrefab default:1
		prefab_name type:#string ui:etxt_prefab_name default:""
	)
	
	rollout params "Prefab" 
	(
		
		dropDownList ddlPrefab "Prefab Type" pos:[10,10] items:prefabs_info.prefabsList align:#left
		edittext etxt_prefab_name "Prefab:" labelOnTop:true readOnly:true
		
		on params open do (
			-- Change the prefab type in the combo box if it exists
			-- If the prefab name is different to the type name, search for the name in the DDL and change the currently selected prefab type
			if (ddlPrefab.selected != etxt_prefab_name.text) then ( 
				local pos = findItem ddlPrefab.items etxt_prefab_name.text
				if (pos != 0) then (
					ddlPrefab.selection = pos
				) else (
					ddlPrefab.selection = 1
				)
			)
		)		
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classPrefabData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		-- When the dropdownList is selected, persist the name in the edittext
		on ddlPrefab selected prefab_selected do
		(
			etxt_prefab_name.text = prefabs_info.prefabsList[prefab]
		)
		
	)
	
	fn exportAsComponent fs obj = (
		local src = ""
			
		src = "data/prefabs/" + prefab_name + ".json"
		
		fs.writeComma()
		fs.writeKeyValue "prefab" src
	)
	
)