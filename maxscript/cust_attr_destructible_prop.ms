classDestructibleProp = attributes classDestructiblePropData attribID: #(0x36abddbb, 0x7478ddddd)
(
		
	parameters main rollout:params
	(
		spawn_prefab type:#string ui:etxt_filename default:""
		drops_warp type:#boolean ui:chk_warp default:false
		fmod_event type:#string ui:etxt_event default:""
	)
	
	rollout params "Destructible Prop" 
	(
		edittext etxt_filename "Spawn Prefab:"  labelOnTop:true width:150
		checkbox chk_warp "Drops Warp"
		edittext etxt_event "FMOD Event:"  labelOnTop:true width:150
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classDestructibleProp
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
	)
	
	fn exportAsComponent fs obj = (
		fs.writeComma()
		fs.writeKey "destructible"
		fs.beginObj()
			fs.writeKeyValue "spawn_prefab" spawn_prefab
			fs.writeComma()
			fs.writeKeyValue "drops_warp" drops_warp
			fs.writeComma()
			fs.writeKeyValue "fmod_event" fmod_event
		fs.endObj() 						
	)
)