mcv_scripts_path = getDir #userStartupScripts
mcv_scripts_path += "/../"
format "Hi from MCV boot %" mcv_scripts_path

filein (mcv_scripts_path + "mcv_common.ms")

-- Exporters and general scripts
filein (mcv_scripts_path + "mesh_exporter.ms")
filein (mcv_scripts_path + "jsonFormatter.ms")
filein (mcv_scripts_path + "material_exporter.ms")
filein (mcv_scripts_path + "scene_exporter.ms")
filein (mcv_scripts_path + "skeleton_exporter.ms")
filein (mcv_scripts_path + "animation_exporter.ms")
filein (mcv_scripts_path + "ui_exporter.ms")
filein (mcv_scripts_path + "ui_surface_types_list.ms")

-- Custom Attributes
filein (mcv_scripts_path + "cust_attrs_common.ms")
filein (mcv_scripts_path + "cust_attr_prefab.ms")
filein (mcv_scripts_path + "cust_attr_collider.ms")
filein (mcv_scripts_path + "cust_attr_death_area.ms")
filein (mcv_scripts_path + "cust_attr_tag.ms")
filein (mcv_scripts_path + "cust_attr_moving_platform.ms")
filein (mcv_scripts_path + "cust_attr_timed_door.ms")
filein (mcv_scripts_path + "cust_attr_basic_mechanism.ms")
filein (mcv_scripts_path + "cust_attr_message_area.ms")
filein (mcv_scripts_path + "cust_attr_ai_controller.ms")
filein (mcv_scripts_path + "cust_attr_rigid_anim_controller.ms")
filein (mcv_scripts_path + "cust_attr_trigger_area.ms")
filein (mcv_scripts_path + "cust_attr_collapsing_props.ms")
filein (mcv_scripts_path + "cust_attr_destructible_prop.ms")
filein (mcv_scripts_path + "cust_attr_mat_surface_type.ms")
filein (mcv_scripts_path + "cust_attr_irradiance_volume.ms")

filein (mcv_scripts_path + "ui_component_manager.ms")


-- To choose the bin path of the project
filein (mcv_scripts_path + "ui_project_directory.ms")
