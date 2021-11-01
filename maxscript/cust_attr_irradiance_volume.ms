classIrradianceVolume = attributes IrradianceVolumeData attribID: #(0x6fade69f, 0x4dda196d)
(
		
	parameters main rollout:params
	(
		filename type:#string ui:etxt_filename default:""
		separation type:#float ui:etxt_separation default:1
	)
	
	rollout params "Irradiance Volume" 
	(
		edittext etxt_filename "Filename:"
		spinner etxt_separation "Separation (m):"
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classIrradianceVolume
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
	)
	
	fn exportAsComponent fs obj = (
		
		if obj == undefined then (
			throw "Irradiane Volume: I need an object to export the scale"
		)
		
		local scaleX = inMeters obj.width
		local scaleY = inMeters obj.height
		local scaleZ = inMeters obj.length
		
		local dims = ceil(scaleX) as string + " " + ceil(scaleY) as string + " " + ceil(scaleZ) as string
		local density = ceil(scaleX / separation) as string + " " + ceil(scaleY / separation) as string + " " + ceil(scaleZ / separation) as string
		
		fs.writeComma()
		fs.writeKey "irradiance_cache"
		fs.beginObj()
			fs.writeKeyValue "filename" filename
			fs.writeComma()
			fs.writeKeyValue "dimensions" dims
			fs.writeComma()
			fs.writeKeyValue "grid_density" density
		fs.endObj() 						
	)
)