
classRigidAnimController = attributes RigidAnimControllerData attribID: #(0x64af22f, 0x116412fa)
(
		
	parameters main rollout:params
	(
		source type:#string ui:editTextSrc default:""		
		autoplay type:#boolean ui:chk_autoplay default:false
		loop type:#boolean ui:chk_loop default:false
		anim_prefabs type:#boolean ui:chk_animate_prefabs default:false
		speed_factor type:#float ui:spn_speed_factor default:1.0
	)
	
	rollout params "Rigid Anim Controller" 
	(
		edittext editTextSrc "Source" labelOnTop:true  height: 22 width:150
		checkbox chk_autoplay "Autoplay"
		checkbox chk_loop "Loop"
		checkbox chk_animate_prefabs "Animate prefabs"
		spinner spn_speed_factor "Speed Factor" range:[0,10,0.1] type:#float

		button btn_remove_comp "Remove Component"
		
		-- Event to remove the component from the object
		on btn_remove_comp pressed do
		(
			try (
				custAttributes.delete $ classRigidAnimController
			)
			catch (
				messageBox ("Error Removing Component:\n" + getCurrentException())
			)
		)
		
	)
	
	fn exportAsComponent fs obj = (
		
		-- Export the component
		fs.writeComma()
		fs.writeKey "animator_controller"
		fs.beginObj()			
			fs.writeKeyValue "src" source
			fs.writeComma()
			fs.writeKeyValue "autoplay" autoplay
			fs.writeComma()
			fs.writeKeyValue "loop" loop
			fs.writeComma()
			fs.writeKeyValue "animate_prefabs" anim_prefabs
			fs.writeComma()
			fs.writeKeyValue "speed_factor" speed_factor
		fs.endObj()
			
	)
)