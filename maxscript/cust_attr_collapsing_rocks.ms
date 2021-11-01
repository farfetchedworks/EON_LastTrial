classCollapsingRocks = attributes CollapsingRocksData attribID: #(0x36abddbb, 0x74786cea)
(
		
	parameters main rollout:params
	(
		spawn_prefab type:#string ui:etxt_filename default:""
		fall_time type:#float ui:spn_fall_time default:1.0
	)
	
	rollout params "Collapsing Rocks" 
	(
		edittext etxt_filename "Spawn Prefab:"  labelOnTop:true width:150
		spinner spn_fall_time "Fall speed:" range:[1,100,10] type:#float
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classCollapsingRocks
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
	)
	
	fn exportAsComponent fs obj = (
		fs.writeComma()
		fs.writeKey "collapsing_rocks"
		fs.beginObj()
			fs.writeKeyValue "spawn_prefab" spawn_prefab
			fs.writeComma()
			fs.writeKeyValue "fall_time" fall_time
		fs.endObj() 						
	)
)