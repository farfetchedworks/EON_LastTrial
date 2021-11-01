
classDeathAreaData = attributes deathAreaData attribID: #(0x5c21c14a, 0x31824169)
(
	
	parameters main rollout:params
	(
	)
	
	rollout params "Death Area" 
	(
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classDeathAreaData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
	)
	
	fn exportAsComponent fs obj = (
		-- Export the component as a single value.
		fs.writeComma()
		fs.writeKey "death_area"
		fs.beginObj()
			fs.writeKeyValue "activated" true
		fs.endObj()
	)
)