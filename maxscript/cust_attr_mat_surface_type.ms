-- Struct created to get the surface type list
struct TSurfaceTypeInfo (
	surfaceTypeList = #(),
	on create do (
		-- Get surface types directory
		local iniFileName = getMaxiniFile()
		local surfaceTypeString = getINISetting iniFileName "MaterialsMCV" "surfaceTypes"
		
		-- get the surface types in the ini setting in the array
		surfaceTypeList = filterString surfaceTypeString "|"
		
	)
)


classMatSurfaceTypeData = attributes matSurfaceTypeData attribID: #(0x323fbc93, 0x2a9bd635)
(

	local surface_type_info = TSurfaceTypeInfo()
	
	parameters main rollout:params
	(
		surface_type type:#integer ui:ddl_surface_type default:1
		surface_type_name type:#string ui:etxt_surface_type_name default:""
	)
	
	rollout params "Surface Type" 
	(
		dropDownList ddl_surface_type "Surface Type" pos:[10,10] items:surface_type_info.surfaceTypeList align:#left
		edittext etxt_surface_type_name "Surface Type:" labelOnTop:true readOnly:true
		
		on params open do (
			-- Store the previously selected object in the edittext
			etxt_surface_type_name.text = surface_type_info.surfaceTypeList[surface_type]
		)	
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete trackViewNodes[#sme][(sme.activeView)][1].reference classMatSurfaceTypeData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		-- When the dropdownList is selected, persist the name in the edittext
		on ddl_surface_type selected surface_type_selected do
		(
			etxt_surface_type_name.text = surface_type_info.surfaceTypeList[surface_type]
		)
	)
	
	fn exportAsComponent fs obj = (
		fs.writeComma()
		fs.writeKeyValue "surface_type" surface_type_name
	)
)