
classTagsData = attributes tagsData attribID: #(0x7054c8ee, 0x24bbf655)
(
	
	parameters main rollout:params
	(
		txt_tags type:#string ui:editTextTags default:""
	)
	
	rollout params "Tags" 
	(
		edittext editTextTags "Tags:" tooltip:"Add tags separated with intro" labelOnTop:true  height: 72
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classTagsData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
		on txt_tags changed tag_list do
		(

		)
	)
	
	fn exportAsComponent fs obj = (
		
		local tags_array = filterString txt_tags "\n"
		fs.writeComma()
		fs.writeKey "tags"
		fs.arrayOfStrings tags_array

	)
)