clearListener()

-- new_vtx = #( pos, normal, uv )
function getIndexOfVertex ht all_vtxs new_vtx = (
	
	-- Create a Unique string Key from the vertex, printing all the coords as "(x1,x2,x3,...)"
	local key = with printAllElements off new_vtx as string
	
	-- Find the string in the hash map
	local idx = ht.Item key
	
	-- Is the key present in the hash table???
	if idx == undefined then (
		-- new_vtx is NOT in all_vtxs
		append all_vtxs new_vtx
		idx = ht.count + 1
		ht.add key idx
	)
	return idx
)

-- http://foundationsofgameenginedev.com/FGED2-sample.pdf
function computeTangentSpace triangleArray vertexArray = (
	
	local vertexCount = vertexArray.count
	local triangleCount = triangleArray.count / 3
	
	-- Allocate temporary storage for tangents and bitangents and initialize to zeros.
	local tangent = #()
	local bitangent = #()

	local i = 1
	while ( i <= vertexCount ) do (
		tangent[i] = [0,0,0]
		bitangent[i] = [0,0,0]
		i = i + 1
	)
	
	-- Calculate tangent and bitangent for each triangle and add to all three vertices.
	local k = 0
	while ( k < triangleCount ) do (
		local i0 = triangleArray[k * 3 + 1]
		local i1 = triangleArray[k * 3 + 2]
		local i2 = triangleArray[k * 3 + 3]
		
		local v0 = vertexArray[ i0 ]
		local v1 = vertexArray[ i1 ]
		local v2 = vertexArray[ i2 ]
		
		-- Positions of the vertex are the 3 first values in the array of data per vertex
		local p0 = [ v0[1], v0[2], v0[3] ]
		local p1 = [ v1[1], v1[2], v1[3] ]
		local p2 = [ v2[1], v2[2], v2[3] ]
		
		-- UV are stored at indices 7 & 8, just after the position (3) and normal( 3)
		local w0 = [ v0[7], v0[8], 0 ]
		local w1 = [ v1[7], v1[8], 0 ]
		local w2 = [ v2[7], v2[8], 0 ]
		
		local e1 = p1 - p0
		local e2 = p2 - p0

		local x1 = w1.x - w0.x
		local x2 = w2.x - w0.x
		local y1 = w1.y - w0.y
		local y2 = w2.y - w0.y
		
		local r = 1.0 / (x1 * y2 - x2 * y1)

		local t = (e1 * y2 - e2 * y1) * r
		local b = (e2 * x1 - e1 * x2) * r
		
		tangent[i0] = tangent[i0] + t
		tangent[i1] = tangent[i1] + t
		tangent[i2] = tangent[i2] + t
		bitangent[i0] = bitangent[i0] + b
		bitangent[i1] = bitangent[i1] + b
		bitangent[i2] = bitangent[i2] + b
		
		k = k + 1
	)
	
	-- Orthonormalize each tangent and calculate the handedness.
	i = 1
	while ( i <= vertexCount ) do (
		local t = tangent[i];
		local b = bitangent[i];

		-- Normal per vertex is stored at offset 4,5,6 of the vertex data
		local v = vertexArray[i]
		local  n = [v[4], v[5], v[6] ]
		
		-- t = 
		local t_parallel_to_n = dot n t
		local t_without_n = t - n * t_parallel_to_n
		t = normalize t_without_n
		
		local t_cross_b = Cross t b
		local t_dot_n = dot t_cross_b n
		local w = 1.0
		if t_dot_n < 0.0 then w = -1.0
			
		join vertexArray[ i ] #(t.x, t.y, t.z, w)
		
		i = i + 1
	)

)

function saveMeshToBinaryFormat obj vtxs idxs vertex_typename mesh_groups ofilename = (

	format "Writing Mesh % with % vtxs and % indices and % groups\n" ofilename vtxs.count idxs.count mesh_groups.count 
	
	makedir (getFilenamePath ofilename)
	local f = fopen ofilename "wb"
	if f == undefined then throw( "Can't create output mesh file " + ofilename )
	
	local magic_header = 0x33888800
	local magic_end_of_header = 0x33888801
	local magic_vertices = 0x33990011
	local magic_indices = 0x33990022
	local magic_mesh_groups = 0x33990033
	local magic_end_of_file = 0x33888888
	
	local version_number = 1
	local primitive_type = 4				-- Triangles = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	local num_primitives = idxs.count / 3
	local bytes_per_float = 4
	-- All vertices have the same format, and are an array of floats, so get the first
	-- vertice and count the number of floats
	local bytes_per_vertex = vtxs[1].count * bytes_per_float
	local bytes_per_index = 2
	if vtxs.count > 65535 then bytes_per_index = 4
	
	-- Write Header
	WriteLong f magic_header
	WriteLong f version_number
	WriteLong f primitive_type
	WriteLong f num_primitives
	
	WriteLong f vtxs.count
	WriteLong f idxs.count
	WriteLong f bytes_per_vertex
	WriteLong f bytes_per_index
	
	-- The vertex type
	local num_bytes_for_vertex_typename = 32
	WriteString f vertex_typename
	-- Add a padding to complete the 32 bytes for the typename
	for n = vertex_typename.count + 1 to ( num_bytes_for_vertex_typename - 1 ) do
		WriteByte f 0
	-- AABB
	-- # materiales
	WriteLong f magic_end_of_header
	
	-- Write Vertices
	local total_vertices_bytes = vtxs.count * bytes_per_vertex
	WriteLong f magic_vertices
	WriteLong f total_vertices_bytes
	for v in vtxs do (
		for c in v do (
			WriteFloat f c
		)
	)
	
	-- Write Indices
	local total_indices_bytes = idxs.count * bytes_per_index
	WriteLong f magic_indices
	WriteLong f total_indices_bytes
	for idx in idxs do (
		-- In the engine, we want the indices, in base 0, not base 1
		local idx0 = idx - 1
		if bytes_per_index == 2 then (
			WriteShort f idx0 #unsigned
		) else (
			WriteLong f idx0 #unsigned
		)
	)
	
	-- Mesh Groups
	if mesh_groups.count > 0 then (
		local bytes_per_mesh_group = 2 * 4 		-- uint32_t x 2
		local total_mesh_groups_bytes = mesh_groups.count * bytes_per_mesh_group
		WriteLong f magic_mesh_groups
		WriteLong f total_mesh_groups_bytes
		for mesh_group in mesh_groups do (
			WriteLong f mesh_group.first_index
			WriteLong f mesh_group.num_indices
		)
	)
	
	-- A final termination mark
	WriteLong f magic_end_of_file
	WriteLong f 0
	
	fclose f
)


function export_mesh given_obj ofilename vertex_format = (
	
	if given_obj == undefined then throw "export_mesh requires a non-null object to export"
	
	local obj = given_obj

	if ofilename == undefined then (

		iniFileName = getMaxiniFile()

		--opath = "e:/code/mcv/bin/data/meshes/"
		--ofilename = opath + obj.name

		local scene_name = "common" --getFilenameFile maxfilename
		-- if scene_name == "" then scene_name = "unnamed"
		
		if vertex_format == "Pos" then (
			opath = getINISetting iniFileName "ExportDirsMCV" "collisionMeshesDir"
			ofilename = opath + scene_name + "/" + obj.name
			ofilename = ofilename + ".cmesh"
		) else (
			opath = getINISetting iniFileName "ExportDirsMCV" "renderMeshesDir"
			ofilename = opath + scene_name + "/" + obj.name
			ofilename = ofilename + ".mesh"
		)
	)
	
	if vertex_format == undefined then vertex_format = "PosNUv" 
	if vertex_format != "Pos" and vertex_format != "PosNUv" then throw ( "Unsupported vertex format" + vertex_format )

	-- Check if the given object if of type editable_mesh
	if classof given_obj != Editable_mesh then (
		-- Do a copy, and convert the copy to an EditableMesh
		obj = copy given_obj
		convertToMesh obj
		obj.name = given_obj.name
	)
	
	local HT = dotNetObject "System.Collections.Hashtable"
	
	-- Do the real export...
	local channelA = 1
	--local channelB = 3
	
	-- Confirm obj has support for the first texture coord set
	if vertex_format != "Pos" and not meshop.getMapSupport obj channelA then throw ("Obj " + given_obj.name + " does NOT have texture coordinates" )
	
	local local_to_world = obj.transform
	local world_to_local = inverse local_to_world
	local max_to_mcv = RotateXMatrix -90
	local world_max_to_mcv_local = world_to_local * max_to_mcv

	local num_faces = getNumFaces obj
	local vtxs_of_face = #(1,3,2)
	
	local all_vtxs = #()
	local all_idxs = #()
	local mesh_groups = #()
	
	-- Find group information
	-- mat #1 - id 1 - Red   200
	-- mat #2 - id 8 - Blue  100
	-- Header | Unique Vertices | Faces Mat1 | Faces Mat 2 | .. |
	local faces_by_material = get_faces_by_material obj
	
	local max_material_index = 0
	for faces_of_material in faces_by_material do (
		max_material_index = max_material_index + 1
		
		if faces_of_material == undefined then continue
	
		local mesh_group = TMeshGroup()
		mesh_group.first_index = all_idxs.count
		
		-- Export each face assigned to that material
		for face_id in faces_of_material do (
		
			local face = getFace obj face_id
			--format "Face % : %\n" face_id face
			
			local world_normals
			local faceA
			if vertex_format != "Pos" then (
				-- The 3 normals (one per vertex) for this face
				world_normals = meshop.getFaceRNormals obj face_id
				--format "  normals % : %\n" face_id normals

				-- Get the 3 indices to access the array of texture coordinates vertices
				faceA = meshop.getMapFace obj channelA face_id
			)

			-- For the 3 vertices of each triangle/face
			for i in vtxs_of_face do (
				
				-- Get the 3d position of the vertex in local space
				local wpos = getVert obj face[i]
				local local_pos = inMeters( wpos * world_max_to_mcv_local )
				--format "  Vtx[%] : %\n" i local_pos

				local local_normal
				local uvA
				if vertex_format != "Pos" then (
					-- Get the normals in local space
					local_normal = normalize ( world_normals[i] * world_max_to_mcv_local.rotationpart )

					-- Get the real texture coordinates associated to the vertex ith of the current face
					uvA = meshop.getMapVert obj channelA faceA[i]
				)
				
				-- 1-t.y -> Swap vertical texture coord to match the sys coord from D3D
				local v
				if vertex_format == "Pos" then (
					v = #( local_pos.x, local_pos.y, local_pos.z )
				) else (
					v = #( local_pos.x, local_pos.y, local_pos.z, local_normal.x, local_normal.y, local_normal.z, uvA.x, 1-uvA.y  )
				)
				
				-- Find v in all_vtxs, if found give the index in all_vtxs array, if not
				-- found, register it, and also return the index
				local new_idx = getIndexOfVertex HT all_vtxs v
				
				append all_idxs new_idx
				
			)
		)
		
		-- End of the mesh group associated to a material
		mesh_group.num_indices = all_idxs.count - mesh_group.first_index
		mesh_group.max_material_index = max_material_index
		if obj.mat != undefined then (
			mesh_group.mat = obj.mat[max_material_index]
		)
		append mesh_groups mesh_group
	)
	
	-- Show all
	/*
	for v in all_vtxs do (
		format "VTX: %\n" v
	)
	for idx in all_idxs do (
		format "IDX: %\n" idx
	)
	*/
	for mesh_group in mesh_groups do (
		format "mesh_group: %\n" mesh_group
	)
	
	if vertex_format != "Pos" then (
		computeTangentSpace all_idxs all_vtxs
		vertex_format = vertex_format + "Tan"
	)
	
	saveMeshToBinaryFormat obj all_vtxs all_idxs vertex_format mesh_groups ofilename
	
	-- If we have been exporting a copy, it's time to remove it
	if given_obj != obj then (
		delete obj
	)

	return true
	
)

--export_mesh $ undefined undefined
--export_mesh $ "e:/code/mcv/bin/data/collision_meshes/convex.cmesh" "Pos"
