classTriggerAreaData = attributes TriggerAreaData attribID: #(0x60a18fec, 0x353d6695)
(
		
	parameters main rollout:params
	(
		area_name type:#string ui:etxt_area_name default:""
		remove_after_enter type:#boolean ui:chk_rem_after_enter default:false
		remove_after_exit type:#boolean ui:chk_rem_after_exit default:false		
	)
	
	
	rollout params "Trigger Area" 
	(
		edittext etxt_area_name "Area Name:" labelOnTop:true height:18
				
		checkbox chk_rem_after_enter "Remove After Enter" tooltip:"Destroy the trigger after the player enters"
		
		checkbox chk_rem_after_exit "Remove After Exit" tooltip:"Destroy the trigger after the player exits"
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classTriggerAreaData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		
	)
	
	fn exportAsComponent fs obj = (
			
		fs.writeComma()
		fs.writeKey "trigger_area"
		fs.beginObj()
			fs.writeKeyValue "area_name" area_name
			fs.writeComma()
			fs.writeKeyValue "remove_after_enter" remove_after_enter
			fs.writeComma()
			fs.writeKeyValue "remove_after_exit" remove_after_exit		
		fs.endObj() 						
	)
)