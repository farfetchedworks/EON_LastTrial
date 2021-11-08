
classMessageAreaData = attributes MessageAreaData attribID: #(0xc554fa7, 0x6980844a)
(
		
	parameters main rollout:params
	(
		message_id type:#string ui:etxt_message_id default:""
		widget_name type:#string ui:etxt_widget_name default:""
		one_time_activation type:#boolean ui:chk_one_time default:true
		pause_game type:#boolean ui:chk_pause_game default:false
		duration type:#float ui:spn_duration default:1.0 
		
	)
	
	
	rollout params "Message Area" 
	(
		edittext etxt_message_id "Message ID:" labelOnTop:true height:18
		
		edittext etxt_widget_name "Widget name:" labelOnTop:true height:18
				
		checkbox chk_one_time "One Time Activation"
		
		checkbox chk_pause_game "Pause Game" tooltip:"If enabled, the message will pause the game until the player closes it"
		
		spinner spn_duration "Duration:" range:[0,100,1] type:#float tooltip:"Duration of the message. If set to 0, the message will be shown until leaving the area"

		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classMessageAreaData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		
	)
	
	fn exportAsComponent fs obj = (
			
		fs.writeComma()
		fs.writeKey "message_area"
		fs.beginObj()
			fs.writeKeyValue "message_id" message_id
			fs.writeComma()
			fs.writeKeyValue "widget_name" widget_name
			fs.writeComma()
			fs.writeKeyValue "one_time_activation" one_time_activation
			fs.writeComma()
			fs.writeKeyValue "pause_game" pause_game
			fs.writeComma()
			fs.writeKeyValue "duration" duration		
		fs.endObj() 						
	)
)