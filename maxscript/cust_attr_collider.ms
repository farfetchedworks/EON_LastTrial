
classColliderData = attributes colliderData attribID: #(0x1a1450f9, 0x16ead0f0)
(
	
	parameters main rollout:params
	(
		collider type:#integer ui:ddlCollider default:1
	)
	
	rollout params "Collider" 
	(
		dropDownList 'ddlCollider' "Collider" pos:[10,10] items:#("Triangle Mesh", "Convex Mesh") align:#left
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classColliderData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
	)
	
	fn exportAsComponent fs obj = (
		local colliderType = ""
		local filename = getFilenameFile maxFileName
		local dir = "data/collision_meshes/" + filename + ".cmesh"
		
		if collider == 1 then colliderType = "trimesh"
		else if collider == 2 then colliderType = "convex"
		
		-- Save the collision as a component with the shape and the collision mesh directory
		fs.writeComma()
		fs.writeKey "collider"
		fs.beginObj()
			fs.writeKeyValue "shape" colliderType
			fs.writeComma()
			fs.writeKeyValue "collision_mesh" dir
		fs.endObj()

	)
)