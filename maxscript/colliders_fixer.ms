-- saves collider for each xref
colliders_idxs = dotNetObject "System.Collections.Hashtable"
colliders_objs = #()

idx = 1
-- fill hashmap
for obj in $* do (
	if classof obj != XRefObject then (
		continue
	)

	if colliders_idxs.Item obj.fileName != undefined then continue

	for child in obj.children do (
		if (isCollider child) then (

			try (
				colliders_idxs.add obj.fileName idx
			)
			catch (
				messageBox ("Error! Object: \"" + obj.Name + "\" has two colliders.\nPlease remove one of them.")
			)

			append colliders_objs child
			idx = idx + 1
		)
	)
)

for obj in $* do (
	if classof obj != XRefObject then (
		continue
	)

	if (isCollider obj) then continue

	if colliders_idxs.Item obj.fileName == undefined then (
		format "No collider found for %\n" obj.name
		continue
	)

	has_collider = false
	for child in obj.children do (
		if (isCollider child) then (
			has_collider = true
		)
	)

	if has_collider == false then (
		idx = colliders_idxs.Item obj.fileName
		collider = colliders_objs[idx]
		local copy_collider = (copy collider)
		append obj.children copy_collider
		copy_collider.transform = obj.transform
		format "Fixed collider for %\n" obj.name
	)
)
