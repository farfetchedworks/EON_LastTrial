fn getMCVRootFolder = (
	-- To be changed
	return "E:/Code/mcv/"
)

fn inMeters pos = (
	if units.SystemType == #centimeters then return ( pos * 0.01 )
	if units.SystemType == #inches then return ( pos / 2.54 )
	if units.SystemType == #meters then return ( pos )
	return pos
)

function max_matrix_to_mcv max_transform = (
    local max_to_mcv = RotateXMatrix -90
    local mcv_to_max = RotateXMatrix 90
    local mcv_transform = mcv_to_max * max_transform * max_to_mcv
    return mcv_transform
)

function getPropOffset obj = (
    local parent = obj.parent
    local child_tm = obj.transform
    local parent_tm = parent.transform
    -- child = parent * delta
    -- inv_parent * child = inv_parent * parent * delta
    -- inv_parent * child = delta
    --local child = inverse( parent_tm ) * child_tm
    
	local child = child_tm * inverse( parent_tm )
    local mcv_child = max_matrix_to_mcv child

    -- format "MaxChild is T:% R:%\n" (inMeters child.pos) child.rotationPart
    format "MCVChild is T:%\n" (inMeters child.pos)
	
	local to_mcv_x = RotateXMatrix -90
	local to_mcv_z = RotateZMatrix -180
	local to_mcv = to_mcv_x
	local rot_to_mcv = to_mcv.rotation
	
	
	format "R:%\n" (child.rotationPart as eulerAngles)

    -- child = delta * parent
    -- child * inv_parent = delta
)

fn isDecalByName n = (
	return ( findString n "decal" != undefined )
)

fn isDecal mat = (
	return mat != undefined and ( isDecalByName mat.name )
)

fn isCollider obj = (
	return obj != undefined and ( findString obj.name "collider" != undefined or findString obj.name "colider" != undefined or findString obj.name "colaider" != undefined)
)

fn isKeepCollider obj = (
	return obj != undefined and ( findString obj.name "keep_collider" != undefined or findString obj.name "keep_colider" != undefined or findString obj.name "keep_colaider" != undefined)
)

fn isTrigger obj = (
	return obj != undefined and (findString obj.name "trigger" != undefined )
)

fn collapseColliders = (

	local previous_parent = getNodeByName "collapsed_parent"
	if (previous_parent != undefined) then (
		delete previous_parent
	)

	local previous_child = getNodeByName "collapsed_collider"
	if (previous_child != undefined) then (
		delete previous_child
	)

	collider_parent = Dummy()
	collider_mesh = editable_mesh()

	parent_material = standardMaterial()
	collider_parent.material = parent_material

	for obj in $* do (

		if (isCollider(obj)) and (isKeepCollider(obj) == false) then (
			local collider_copy = copy obj
			attach collider_mesh collider_copy
		)
	)

	collider_mesh.name = "collapsed_collider"
	collider_parent.name = "collapsed_parent"
	append collider_parent.children collider_mesh
)

-- -------------------------------------------------------------
struct TMeshGroup (
	first_index = 0,
	num_indices = 0,
	max_material_index = 0,
	mat
)

-- #( #(face_id using mat1), undef, undef ... #(faces using mat8))
function get_faces_by_material obj = (
	local results = #()
	local num_faces = getNumFaces obj
	for face_id = 1 to num_faces do (
		local mat_id = getfacematid obj face_id 
		if results[ mat_id ] == undefined then (
			results[ mat_id ] = #()
		)
		append results[ mat_id ] face_id
	)
	return results
)