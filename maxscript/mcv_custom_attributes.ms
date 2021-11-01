
classPrefabData = attributes prefabData 
(
	
	parameters main rollout:params
	(
		prefab type:#integer ui:dllPrefab default:1
	)
	
	rollout params "Prefab" 
	(
		dropDownList 'dllPrefab' "Prefab Type" pos:[10,10] items:#("Capsule", "Enemy", "Fire") align:#left
	)
	
	fn exportAsComponent fs obj = (
		local src = ""
		if prefab == 1 then src = "data/scenes/capsule.json"
		else if prefab == 2 then src = "data/scenes/enemy.json"
		else if prefab == 3 then src = "data/scenes/fire.json"
		
		fs.writeComma()
		fs.writeKeyValue "prefab"  src
	)
)
-- custAttributes.add $ classPrefabData

/*
classTagsData = attributes tagsData 
(
	
	parameters main rollout:params
	(
		tag type:#string ui:strTag default:""
	)
	
	rollout params "Prefab" 
	(
		dropDownList 'dllPrefab' "Prefab Type" pos:[10,10] items:#("Capsule", "Enemy", "Fire") align:#left
	)
	
	fn exportAsComponent fs = (
		local src = ""
		if prefab == 1 then src = "data/scenes/capsule.json"
		else if prefab == 2 then src = "data/scenes/enemy.json"
		else if prefab == 3 then src = "data/scenes/fire.json"
		if tag
		fs.writeComma()
		fs.writeKeyValue "prefab"  src
	)
	
)

*/