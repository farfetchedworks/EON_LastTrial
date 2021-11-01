
classAIController = attributes AIControllerData attribID: #(0x54af11ee, 0x133492ed)
(
	
	parameters main rollout:params
	(
		can_patrol type:#boolean ui:chk_can_patrol default:true
		can_observe type:#boolean ui:chk_can_observe default:false
		random_patrol type:#boolean ui:chk_random_patrol default:true
		waypoints_array type:#stringTab tabSize:0 tabSizeVariable:true default:""	-- To persist the waypoints in an array (listboxes do not persist values)
	)
	
	rollout params "AI Controller" 
	(

		pickButton pbt_choosewpt "Select waypoint"
		
		listbox lst_waypoints "Waypoints" toolTip:"Double click to remove a waypoint"
		
		checkbox chk_can_patrol "Can Patrol"

		checkbox chk_can_observe "Can Look Around"
		
		checkbox chk_random_patrol "Random Patrol"

		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classAIController
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		-- When an object is picked, its name (or position) is saved to the list
		on pbt_choosewpt picked curr_waypoint do
		(
			lst_waypoints.items = append lst_waypoints.items curr_waypoint.name
			
			-- If the waypoints array is full of undefined, initialize it and then add the waypoint
			if(waypoints_array[1]==undefined)then (
				waypoints_array = #()
			)
			waypoints_array = append waypoints_array curr_waypoint.name
		)
		
		-- If an item is double clicked, it is eliminated from the list
		on lst_waypoints doubleClicked wpt do
		(
			temp = lst_waypoints.items
			deleteItem temp wpt
			deleteItem waypoints_array wpt			-- remove the wpt from the persistence list
			lst_waypoints.items = temp
		)
		
		-- Assign the waypoints list to the list box items array when the rollout opens
		on params open do (
			if (waypoints_array[1]==undefined) then(
				lst_waypoints.items = #()
			)else(
				lst_waypoints.items = waypoints_array as array
			)
		)		
			
		
	)
	
	fn exportAsComponent fs obj = (
		
		local wpt_positions = #()
		
		-- Create array with the positions
		for wpt in waypoints_array do (
			local wpt_node = getNodeByName wpt
			local wpt_position = inMeters(wpt_node.pos)
			local wpt_pos_string = wpt_position.x as string + " " + wpt_position.z as string + " " + ((-1)*wpt_position.y) as string
			wpt_positions = append wpt_positions wpt_pos_string
		)
		
		-- Export the component
		fs.writeComma()
		fs.writeKey "ai_controller_base"
		fs.beginObj()			
			fs.writeKeyValue "can_patrol" can_patrol
			fs.writeComma()
			fs.writeKeyValue "can_observe" can_observe
			fs.writeComma()
			fs.writeKeyValue "random_patrol" random_patrol
			if (waypoints_array.count > 0) then (
				fs.writeComma()
				fs.writeKey "waypoints"			
				fs.arrayOfStrings wpt_positions
			)
		fs.endObj()
			
	)
)