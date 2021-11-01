
classBasicMechanismData = attributes BasicMechanismData attribID: #(0x6a1194e4, 0x6916f1ef)
(
	
	local dest_position
	local button_position
	
	parameters main rollout:params
	(
		move_speed type:#float ui:spn_move_speed default:1.0
		end_pos type:#string ui:etxt_end_pos default:""
		button_pos type:#string ui:etxt_button_pos default:""
		other_target_name type:#string ui:etxt_other_target default:""
	)
	
	rollout params "Basic Mechanism" 
	(

		pickButton pbt_choosedest "Select End Position"
		edittext etxt_end_pos "End Position:" labelOnTop:true readOnly:true
		
		pickButton pbt_choosebuttonpos "Select Button Position"
		edittext etxt_button_pos "Button Position:" labelOnTop:true readOnly:true
		
		pickButton pbt_chooseothertarget "Select Other Button Target"
		edittext etxt_other_target "Other Target Name:" labelOnTop:true readOnly:true
		
		spinner spn_move_speed "Speed:" range:[1,100,10] type:#float

		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classBasicMechanismData
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
		
		on pbt_chooseothertarget picked other_target_mesh do
		(
			other_button_target_name = other_target_mesh.name
			etxt_other_target.text = other_button_target_name
		)
			
		
	)
	
	fn exportAsComponent fs obj = (
		
		--if end_pos != "" and button_pos != "" then (
		
		fs.writeComma()
		fs.writeKey "basic_mechanism"
		fs.beginObj()
			fs.writeKeyValue "speed" move_speed
			fs.writeComma()
			if(end_pos!="") then (
				fs.writeKeyValue "end_position" end_pos
				fs.writeComma()
			)
			if(button_pos!="") then (
				fs.writeKeyValue "button_position" button_pos
				fs.writeComma()
			)
			fs.writeKeyValue "other_target_name" other_target_name				
		fs.endObj()
-- 		)
-- 		else(
-- 			messageBox ("You must select a destination and button position for the mechanism\n")
-- 		) 
-- 						
	)
)