
classTimedDoorData = attributes timedDoorData attribID: #(0x4f6a4385, 0x1437e0c4)
(
	
	local dest_position
	local button_position
	local dest_pos_txt
	local button_pos_txt
	
	parameters main rollout:params
	(
		move_speed type:#float ui:spn_move_speed default:1.0
		closing_time type:#float ui:spn_closing_time default:1.0
		end_pos type:#string ui:etxt_end_pos default:""
		button_pos type:#string ui:etxt_button_pos default:""		
	)
	
	rollout params "Timed Door" 
	(

		pickButton pbt_choosedest "Select End Position"
		edittext etxt_end_pos "End Position:" labelOnTop:true readOnly:true
		pickButton pbt_choosebuttonpos "Select Button Position"
		edittext etxt_button_pos "Button Position:" labelOnTop:true readOnly:true
		spinner spn_move_speed "Speed:" range:[1,100,10] type:#float
		spinner spn_closing_time "Closing time:" range:[1,100,10] type:#float

		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classTimedDoorData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		on pbt_choosedest picked dest_mesh do
		(
			dest_position = inMeters(dest_mesh.pos)
			etxt_end_pos.text = dest_position.x as string + " " + dest_position.z as string + " " + ((-1)*dest_position.y) as string
			
		)
		
		on pbt_choosebuttonpos picked button_mesh do
		(
			button_position = inMeters(button_mesh.pos)
			etxt_button_pos.text = button_position.x as string + " " + button_position.z as string + " " + ((-1)*button_position.y) as string
		)
			
		
	)
	
	fn exportAsComponent fs obj = (
		
		if end_pos != "" and button_pos != "" then (
		
		fs.writeComma()
		fs.writeKey "timed_door"
		fs.beginObj()
			fs.writeKeyValue "speed" move_speed
			fs.writeComma()
			fs.writeKeyValue "closing_time" closing_time
			fs.writeComma()
			fs.writeKeyValue "end_position" end_pos
			fs.writeComma()
			fs.writeKeyValue "button_position" button_pos			
		fs.endObj()
		)
		else(
			messageBox ("You must select a destination and button position for the door\n")
		) 
						
	)
)