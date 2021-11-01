
classMovingPlatformData = attributes movingPlatformData attribID: #(0x63feafa0, 0xed9e143)
(
	
	local dest_position
	
	parameters main rollout:params
	(
		move_speed type:#float ui:spn_move_speed default:1.0
	)
	
	rollout params "Moving Platform" 
	(

		pickButton pbt_choosedest "Select Destination"
		spinner spn_move_speed "Speed:" range:[1,100,10] type:#float

		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classMovingPlatformData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		on pbt_choosedest picked dest_mesh do
		(
			dest_position = inMeters(dest_mesh.pos)
		)
			
		
	)
	
	fn exportAsComponent fs obj = (
		
		local dest_vector
		dest_vector = dest_position.x as string + " " + dest_position.z as string + " " + ((-1)*dest_position.y) as string
		
		fs.writeComma()
		fs.writeKey "moving_platform"
		fs.beginObj()
			fs.writeKeyValue "speed" move_speed
			fs.writeComma()
			fs.writeKeyValue "end_position" dest_vector			
		fs.endObj()
	)
)