-- Struct created to get the surface types list
struct TSurfTypeInfo (
	surfaceTypeList = #(),
	on create do (
		-- Get surface types directory
		local iniFileName = getMaxiniFile()
		local surfaceTypeString = getINISetting iniFileName "MaterialsMCV" "surfaceTypes"
		
		-- Store the surface types in the ini setting to the array
		surfaceTypeList = filterString surfaceTypeString "|"
		
	)
)


utility UtilSurfaceTypes "Physical Surfaces"
(
	
	
	rollout rSurfaceTypes "Physical Surfaces"
	(
		local surface_type_info = TSurfTypeInfo()
		
		edittext txt_surfacetype "Surface Type:" labelOnTop:true  height: 18		
		button btn_addsurfacetype "Add surface type"
		listbox lst_surfacetypes "Surface Types" items:surface_type_info.surfaceTypeList toolTip:"Double click to remove a surface type"
		
		-- If an item is double clicked, it is eliminated from the list
		on lst_surfacetypes doubleClicked srf_type do
		(
			temp = lst_surfacetypes.items
			deleteItem temp srf_type
			lst_surfacetypes.items = temp
			
			local srf_types_string = ""
			-- Store the new list in the ini file
			for n = 1 to lst_surfacetypes.items.count do (
				srf_types_string = srf_types_string + lst_surfacetypes.items[n]
				if n < lst_surfacetypes.items.count then
					srf_types_string = srf_types_string + "|"
			)
			
			local iniFileName = getMaxiniFile()	
			setINISetting iniFileName "MaterialsMCV" "surfaceTypes" srf_types_string
		)
		
		-- adds the surface type to the surface types list
		on btn_addsurfacetype pressed do
		(			
			lst_surfacetypes.items = append lst_surfacetypes.items txt_surfacetype.text
			
			local iniFileName = getMaxiniFile()
			local srf_types_string = getINISetting iniFileName "MaterialsMCV" "surfaceTypes"
			srf_types_string = srf_types_string + "|" + txt_surfacetype.text
				
			setINISetting iniFileName "MaterialsMCV" "surfaceTypes" srf_types_string			
		)

	)
	
	on UtilSurfaceTypes open do
	(
		addRollout rSurfaceTypes
	) 
	on UtilSurfaceTypes close do
	(
		removeRollout rSurfaceTypes
	) 
	
)
