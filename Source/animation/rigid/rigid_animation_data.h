#pragma once
#include "resources/resources.h"

class TCoreAnimationData : public IResource
{
public:

	struct TCoreTrackHeader
	{
		static constexpr uint32_t valid_magic = 0x6a6a6a01;
		static constexpr uint32_t valid_version = 1;
		static constexpr uint32_t max_obj_name = 32;
		static constexpr uint32_t max_property_name = 16;
		static constexpr uint32_t max_keys_type = 16;

		uint32_t magic = 0;
		uint32_t version = 0;
		uint32_t num_keys = 0;
		uint32_t bytes_per_key = 0;
		float    frame_rate = 0.0f;
		float    min_time = 0.0f;
		float    max_time = 0.0f;
		uint32_t var_data_size = 0;
		char     obj_name[max_obj_name];
		char     property_name[max_property_name];
		char     keys_type_name[max_keys_type];

		bool isValid() const
		{
			return magic == valid_magic && version == valid_version;
		}

		void renderInMenu() const
		{
			ImGui::Text("Min Time: %f", min_time);
			ImGui::Text("Max Time: %f", max_time);
			ImGui::Text("Frame Rate: %f", frame_rate);
			ImGui::Text("Num Keys: %f (%d bytes/key)", num_keys, bytes_per_key);
			if(var_data_size)
				ImGui::Text("Variable Data Size: %d", var_data_size);
		}
	};

	// Main Header
	// Track0.Header
	// Track0.Keys...
	//   prs0 0f
	//   prs0 1f
	//   prs0 2f
	//   prs0 3f
	//   ...
	// Track1.Header
	// Track1.Keys...
	//   prs1 0f
	//   prs1 1f
	//   prs1 2f
	//   prs1 3f
	//   ...

	struct TCoreTrack : public TCoreTrackHeader
	{
		TBuffer keys;
		TBuffer var_data;

		enum class eKeyType {
			UNDEFINED,
			TRANSFORM,
			TIMED_NOTE
		};
		eKeyType keys_type = eKeyType::UNDEFINED;

		template< class T >
		const T& getKey(uint32_t idx) const
		{
			assert(idx >= 0 && idx < num_keys);
			assert(bytes_per_key == sizeof(T));
			return *((const T*)keys.data() + idx);
		}
		void renderInMenu() const
		{
			TCoreTrackHeader::renderInMenu();
			if (ImGui::TreeNode("Keys..."))
			{
				for (uint32_t i = 0; i < num_keys; ++i)
				{
					if (keys_type == eKeyType::TRANSFORM)
					{
						const CTransform& transform = getKey<CTransform>(i);
						ImGui::Text("Pos: %f %f %f", transform.getPosition().x, transform.getPosition().y, transform.getPosition().z);
					}
					else if (keys_type == eKeyType::TIMED_NOTE)
					{
						const TTimedNote& note = getKey<TTimedNote>(i);
						ImGui::Text("At: %f %d %d %s", note.time, note.offset, note.size, note.getText(var_data));
					}
					//ImGui::Text("Rot: %f %f %f %f");
				}
				ImGui::TreePop();
			}
		}
	};

	struct TCoreHeader
	{
		static constexpr uint32_t valid_magic = 0x6a6a6a00;
		static constexpr uint32_t valid_version = 2;
		uint32_t magic = 0;
		uint32_t version = 0;
		uint32_t ntracks = 0;
		uint32_t padding = 0;
		float    min_time = 0.0f;
		float    max_time = 0.0f;
		uint32_t padding1 = 0;
		uint32_t padding2 = 0;
		bool isValid() const
		{
			return magic == valid_magic && version == valid_version;
		}
	};

	struct TTimedNote
	{
		float    time = 0.0f;
		uint32_t offset = 0;
		uint32_t size = 0;

		const char* getText( const TBuffer& b ) const {
			assert(offset >= 0 && offset + size <= b.size());
			return (const char*)b.data() + offset;
		}
	};

	TCoreHeader             header;
	std::vector<TCoreTrack> tracks;

	bool read(IDataProvider& dp)
	{
		dp.read(header);
		if (!header.isValid())
			return false;

		tracks.resize(header.ntracks);
		for (uint32_t i = 0; i < header.ntracks; ++i)
		{
			TCoreTrack& track = tracks[i];
			dp.readBytes(sizeof(TCoreTrackHeader), &track);
			assert(track.isValid());

			if (strcmp(track.keys_type_name, "CTransform") == 0) {
				track.keys_type = TCoreTrack::eKeyType::TRANSFORM;
				assert(track.bytes_per_key == sizeof(CTransform));
			}
			else if (strcmp(track.keys_type_name, "TTimedNote") == 0) {
				track.keys_type = TCoreTrack::eKeyType::TIMED_NOTE;
				assert(track.bytes_per_key == sizeof(TTimedNote));
			}
			else
				fatal("Unsupported key type name %s", track.keys_type_name);

			size_t total_bytes = track.bytes_per_key * track.num_keys;
			track.keys.resize(total_bytes);
			dp.readBytes(total_bytes, track.keys.data());

			if (track.var_data_size) {
				track.var_data.resize(track.var_data_size);
				dp.readBytes(track.var_data_size, track.var_data.data());
			}
		}

		return true;
	}

	bool renderInMenu() const override
	{
		for (auto& t : tracks)
		{
			std::string id = t.obj_name + std::string(".") + t.property_name + std::string(".") + t.keys_type_name;
			if (ImGui::TreeNode(id.c_str())) {
				t.renderInMenu();
				ImGui::TreePop();
			}
		}
		return false;
	}

};