classEmissiveMorse = attributes classEmissiveMorseData attribID: #(0x36ab0178, 0x56b2d146e)
(
		
	parameters main rollout:params
	(
		period type:#float ui:n_period default:0.1
		evalue type:#float ui:n_evalue default:1
		original type:#boolean ui:b_original default:true
		randomize type:#boolean ui:b_randomize default:false
		chars type:#string ui:txt_chars default:"... --- ...xxxxx"
	)
	
	rollout params "Emissive Morse" 
	(
		spinner n_period "Unit period:" range:[0.01,2,0.01] type:#float
		spinner n_evalue "Emissive Value:" range:[0.0,1000,0.1] type:#float
		checkbox b_original "Use Original:"
		checkbox b_randomize "Randomize:"
		edittext txt_chars "Text in MORSE:" labelOnTop:true width:150
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classEmissiveMorse
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
	)
	
	fn exportAsComponent fs obj = (
		fs.writeComma()
		fs.writeKey "emissive_blink"
		fs.beginObj()
			fs.writeKeyValue "unitPeriod" period
			fs.writeComma()
			fs.writeKeyValue "charString" chars
			fs.writeComma()
			fs.writeKeyValue "useOriginal" original
			fs.writeComma()
			fs.writeKeyValue "emissiveValue" evalue
			fs.writeComma()
			fs.writeKeyValue "randomize" randomize
		fs.endObj() 						
	)
)

classAnimationLauncher = attributes classAnimationLauncherData attribID: #(0x361216aa, 0x12bba4ff1)
(
		
	parameters main rollout:params
	(
		variable type:#string ui:txt_var default:""
	)
	
	rollout params "Animation Launcher" 
	(
		edittext txt_var "FSM Animation Variable:" labelOnTop:true width:150
		
		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classAnimationLauncherData
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
	)
	
	fn exportAsComponent fs obj = (
		fs.writeComma()
		fs.writeKey "anim_launcher"
		fs.beginObj()
			fs.writeKeyValue "var" variable
		fs.endObj() 						
	)
)