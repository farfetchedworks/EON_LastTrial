classCollapsingProps = attributes CollapsingPropsData attribID: #(0x36abddbb, 0x74786cea)
(
		
	parameters main rollout:params
	(
		spawn_prefab type:#string ui:etxt_spawn_prefab default:""
		fmod_event type:#string ui:etxt_fmod_event default:""
		type type:#string ui:etxt_type default:""
		fall_time type:#float ui:spn_fall_time default:1.0
	)
	
	rollout params "Collapsing Rocks" 
	(
		edittext etxt_spawn_prefab "Spawn Prefab:"  labelOnTop:true width:150
		edittext etxt_type "Type:"  labelOnTop:true width:150
		edittext etxt_fmod_event "FMOD event:"  labelOnTop:true width:150
		
		spinner spn_fall_time "Fall speed:" range:[1,100,10] type:#float
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ CollapsingPropsData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
	)
	
	fn exportAsComponent fs obj = (
		fs.writeComma()
		fs.writeKey "collapsing_props"
		fs.beginObj()
			fs.writeKeyValue "spawn_prefab" spawn_prefab
			fs.writeComma()
			fs.writeKeyValue "type" type
			fs.writeComma()
			fs.writeKeyValue "fmod_event" fmod_event
			fs.writeComma()
			fs.writeKeyValue "fall_time" fall_time
		fs.endObj() 						
	)
)