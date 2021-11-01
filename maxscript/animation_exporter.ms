clearListener()

fn writeStringFixedLength fs str fixed_length = (
	local  str_len =  str.count
	if  str_len > fixed_length - 1 then throw ("String " + str + " is too long. Max size is " + fixed_length)
	writeString fs str
	-- End the 32 bytes with zeros
	local j = str_len + 1
	while j < fixed_length do (
		writeByte fs 0
		j = j + 1
	)	
)

fn asSeconds nframe = (
	return (nframe as integer) / 4800.0
)

struct TAnimationTrack (
	obj_name,
	property_name,
	keys_type,
	frame_rate,
	min_frame,
	max_frame,
	keys = #(),
	
	var_data = #(),
	var_data_size = 0,
	
	fn writeToFile fs = (
		
		local bytes_per_float = 4
		local bytes_per_key = keys[1].count * bytes_per_float
		
		writeLong fs 0x6a6a6a01			-- Magic
		writeLong fs 1					-- version number
		writeLong fs keys.count
		writeLong fs bytes_per_key		-- in bytes
		
		local min_time = asSeconds( min_frame )
		local max_time = asSeconds( max_frame )
		
		writeFloat fs frame_rate
		writeFloat fs min_time
		writeFloat fs max_time
		writeLong fs var_data_size
		
		writeStringFixedLength fs obj_name 32
		writeStringFixedLength fs property_name 16
		writeStringFixedLength fs keys_type 16
		
		format "%.%.% from % to %, % keys [% var]\n" obj_name property_name keys_type min_time max_time keys.count var_data_size
		
		for k in keys do (
			for v in k do (
				if classof v == Float then writeFloat fs v
				else if classof v == Integer then writeLong fs v
				else throw ("Don't know how to write key component " + v)
			)
		)
		
		for s in var_data do (
			format "Write extra data %\n" s
			if classof s == String then WriteString fs s;
			else throw ("Don't know how to write data " + v)
		)
	
	),
	
	fn saveVarData new_data = (
		local new_start = var_data_size
		local new_size = new_data.count 		-- One extra byte for the null terminator
		var_data_size = var_data_size + new_size + 1
		append var_data new_data
		return #( new_start, new_size )
	)
	
)


struct TAnimationExporter (
	obj,
	frame_rate = 5,
	tracks = #(),
	
	fn exportTransformTrack range = (
		local t = TAnimationTrack()
		t.min_frame = range[1]
		t.max_frame = range[2]
		t.obj_name = obj.name
		t.frame_rate = frame_rate
		t.property_name = "transform"
		t.keys_type = "CTransform"
		local nsamples = ( t.max_frame.frame - t.min_frame.frame ) / frame_rate
		local nsample
		for nsample = 1 to nsamples do (
			local nframe = t.min_frame + ( t.max_frame.frame - t.min_frame.frame ) * ( nsample - 1 )/ ( nsamples - 1)
			-- format "Sampling at frame %\n" frame_time
		
			local max_transform = at time nframe obj.transform
			
			local max_to_mcv = RotateXMatrix -90
			local mcv_to_max = RotateXMatrix 90
			local mcv_transform = mcv_to_max * max_transform * max_to_mcv
		
			local mcv_pos_meters = inMeters( mcv_transform.position )
			local mcv_scale = mcv_transform.scale
			local mcv_rot = mcv_transform.rotation
			
			-- Matches our CTransform = Q,T,S
			local key = #();
			key = join key #(mcv_rot.x, mcv_rot.y, mcv_rot.z, -mcv_rot.w)
			key = join key #(mcv_pos_meters.x, mcv_pos_meters.y, mcv_pos_meters.z)
			key = join key #(mcv_scale.x, mcv_scale.y, mcv_scale.z)
			append t.keys key
			
		)
		
		if t.keys.count == 0 then (
			format "obj % is not moving!!" obj.name
			return undefined
		)
		
		append tracks t
	),
	
	fn exportNoteTracks = (
		local ntracks = numNoteTracks obj
		for track_idx = 1 to ntracks do (
			local note_track = getNoteTrack obj track_idx
			if note_track.keys.count == 0 then continue
			
			local t = TAnimationTrack()
			t.min_frame = note_track.keys[1].time
			t.max_frame = note_track.keys[ note_track.keys.count ].time
			t.obj_name = obj.name
			t.frame_rate = 1
			t.property_name = "notes"
			t.keys_type = "TTimedNote"
			for note in note_track.keys do (
				local var_offset_size = t.saveVarData( note.value )
				local key = #( asSeconds(note.time), var_offset_size[1], var_offset_size[2] )
				format "obj % notetrack % note at % : % Key:%\n" obj.name track_idx note.time note.value key
				append t.keys key
			)
			append tracks t
		)
	),
	
	fn expandAnimationRange curr_range controller = (
		local num_keys = controller.keys.count
		if num_keys == 0 then return curr_range
		local min_frame = controller.keys[1].time
		local max_frame = controller.keys[controller.keys.count].time
		if curr_range.count == 0 then return #(min_frame, max_frame)
		if min_frame < curr_range[1] then curr_range[1] = min_frame
		if max_frame > curr_range[2] then curr_range[2] = max_frame
		return curr_range
	),
	
	fn exportObj new_obj = (
		if new_obj == undefined then return undefined
		obj = new_obj
		
		local time_range = #()
		time_range = expandAnimationRange time_range obj.pos.controller
		time_range = expandAnimationRange time_range obj.rotation.controller
		
		exportTransformTrack time_range
		exportNoteTracks()
	),
	
	fn writeAnimation filename = (
		
		local ntracks = tracks.count
		if ntracks == 0 then (
			throw "No tracks to export"
			return undefined
		)
		
		local iniFileName = getMaxiniFile()
		local localBinPath = getINISetting iniFileName "ExportDirsMCV" "binDir"
		local outfilename = localBinPath + "data/animations/" + filename + ".anim"
		
		local fs = fopen outfilename "wb"
		if fs == undefined then throw ("Can't create output anim filename " + outfilename )
		
		local global_min_frame = tracks[1].min_frame
		local global_max_frame = tracks[1].max_frame 
		for t in tracks do (
			if t.min_frame < global_min_frame then global_min_frame = t.min_frame
			if t.max_frame > global_max_frame then global_max_frame = t.max_frame
		)
		local global_min_time = asSeconds global_min_frame 
		local global_max_time = asSeconds global_max_frame 
		
		-- Save header
		writeLong fs 0x6a6a6a00		-- Magic
		writeLong fs 2						-- # Version
		writeLong fs ntracks
		writeLong fs 0						-- dummy
		writeFloat fs global_min_time
		writeFloat fs global_max_time
		writeLong fs 0						-- dummy
		writeLong fs 0						-- dummy
		
		for t in tracks do (
			t.writeToFile fs
		)
		
		fclose fs
		
	)
	
)

--t = TAnimationExporter()
--t.exportObj $Cylinder001
--t.exportObj $Sphere001
--t.writeAnimation "data/animations/demo00.anim"