-- NOTE: When creating a custom attribute always consider the following:
-- 1) Generate an attribute ID. Open the listener and execute the method
-- genClassID()
-- This will return an ID such as #(0x2d8bf3fb, 0x22b1a736)
--
-- 2) Create the custom attribute definition and add an attrib ID to the custom attribute, like this:
-- classPrefabData = attributes prefabData attribID: #(0x2d8bf3fb, 0x22b1a736) 
-- In this way custom attributes are created as global and all instances will share the same definition
-- Even when there are changes in the script, when re-opening Max or when copying nodes


utility ComponentManager "Component Manager"
(
	
	rollout componentManager "Add Component"
	(
		pickButton 'pbt_choosemesh' "Select Mesh"
		
		label lab0 "Common" align:#left
		
		button btn_collider "Collider" align:#left
		button btn_prefab "Prefab" align:#left
		button btn_tag "Tags" align:#left
		
		label lab1 "Gameplay" align:#left
		button btn_death_area "Death Area" align:#left
		--button btn_moving_platform "Moving Platform" align:#left
		--button btn_timed_door "Timed Door" align:#left
		--button btn_basic_mechanism "Basic Mechanism" align:#left
		button btn_message_area "Message Area" align:#left
		button btn_trigger_area "Trigger Area" align:#left
		button btn_collapsing_props "Collapsing Props" align:#left
		button btn_destructible_prop "Destructible Prop" align:#left
		button btn_anim_laucher "Animation Launcher" align:#left
		
		label lab2 "Controllers" align:#left
		button btn_ai_controller "AI Controller" align:#left
		button btn_rigid_controller "Rigid Anim Controller" align:#left
		
		label lab3 "Render" align:#left
		button btn_mat_surface_type "Material Surface Type" align:#left
		button btn_irradiance_volume "Irradiance Volume" align:#left
		button btn_emissive_morse "Emissive Morse" align:#left
		
		--custAttributes.add $ classPrefabData
		
		-- Adds a Collider component to the current selected object
		on btn_collider pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classColliderData
				custAttributes.add $ classColliderData
				
				MessageBox ("Added Collider")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a Prefab component to the current selected object
		on btn_prefab pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classPrefabData
				custAttributes.add $ classPrefabData
				
				MessageBox ("Added Prefab")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)		
		
		-- Adds a death area component to the current selected object
		on btn_death_area pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classDeathAreaData
				custAttributes.add $ classDeathAreaData
				
				MessageBox ("Added Death Area")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)		
		
		-- Adds a tag component to the current selected object
		on btn_tag pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classTagsData
				custAttributes.add $ classTagsData
				
				MessageBox ("Added Tags")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a moving platform component to the current selected object
		/*on btn_moving_platform pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classMovingPlatformData
				custAttributes.add $ classMovingPlatformData
				
				MessageBox ("Added Moving Platform")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a timed door component to the current selected object
		on btn_timed_door pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classTimedDoorData
				custAttributes.add $ classTimedDoorData
				
				MessageBox ("Added Timed Door")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a basic mechanism component to the current selected object
		on btn_basic_mechanism pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classBasicMechanismData
				custAttributes.add $ classBasicMechanismData
				
				MessageBox ("Added Basic Mechanism")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)*/
		
		-- Adds a message area component to the current selected object
		on btn_message_area pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classMessageAreaData
				custAttributes.add $ classMessageAreaData
				
				MessageBox ("Added Message Area")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds an anim launcher component to the current selected object
		on btn_anim_laucher pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classAnimationLauncher
				custAttributes.add $ classAnimationLauncher
				
				MessageBox ("Added Animation Launcher")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		
		-- Adds an AI Controller component to the current selected object
		on btn_ai_controller pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classAIController
				custAttributes.add $ classAIController
				
				MessageBox ("Added AI Controller")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		on btn_rigid_controller pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classRigidAnimController
				custAttributes.add $ classRigidAnimController
				
				MessageBox ("Added Rigid Anim Controller")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a trigger area component to the current selected object
		on btn_trigger_area pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classTriggerAreaData
				custAttributes.add $ classTriggerAreaData
				
				MessageBox ("Added Trigger Area")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)

		-- Adds a collapsing props component to the current selected object
		on btn_collapsing_props pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classCollapsingProps
				custAttributes.add $ classCollapsingProps
				
				MessageBox ("Added Collapsing Props")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a material surface type component to the current selected object
		on btn_mat_surface_type pressed do
		(
			try (
				if trackViewNodes[#sme][(sme.activeView)][1].reference == undefined then throw "You must select a material in the Slate Material Editor"
				gc()
				
				custAttributes.delete trackViewNodes[#sme][(sme.activeView)][1].reference classMatSurfaceTypeData
				custAttributes.add trackViewNodes[#sme][(sme.activeView)][1].reference classMatSurfaceTypeData
				
				MessageBox ("Added Material Surface Type")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a irradiance component to the current selected object
		on btn_irradiance_volume pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classIrradianceVolume
				custAttributes.add $ classIrradianceVolume
				
				MessageBox ("Added Irradiance Volume")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a destructible component to the current selected object
		on btn_destructible_prop pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classDestructibleProp
				custAttributes.add $ classDestructibleProp
				
				MessageBox ("Added Destructible Prop")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
		-- Adds a emissive morse component to the current selected object
		on btn_emissive_morse pressed do
		(
			try (
				if $ == undefined then throw "You must select an object"
				gc()
				custAttributes.delete $ classEmissiveMorse
				custAttributes.add $ classEmissiveMorse
				
				MessageBox ("Added Emissive Morse")
			)
			catch (
				messageBox ("Error Adding Component:\n" + getCurrentException())
			)
		)
		
	)
	
	-- ...
	on ComponentManager open do
	(
		--- read from an .ini the current directory path...
		addRollout componentManager
	) 
	on ComponentManager close do
	(
		removeRollout componentManager
	) 
	
)
