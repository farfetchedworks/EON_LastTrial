#include "mcv_platform.h"
#include "physical_material.h"

static NamedValues<EPhysMatType>::TEntry physical_mat_types[] = {
  {EPhysMatType::MARBLE, "Marble"},
  {EPhysMatType::ROCK, "Rock"},
  {EPhysMatType::GRAVEL, "Gravel"},
  {EPhysMatType::GRASS, "Grass"},
  {EPhysMatType::WATER, "Water"}
};

static NamedValues<EPhysMatType> physical_mat_names(physical_mat_types, sizeof(physical_mat_types) / sizeof(NamedValues<EPhysMatType>::TEntry));

CPhysicalMaterial::CPhysicalMaterial(const std::string& in_type)
{
	type = physical_mat_names.valueOf(in_type.c_str());
}

const char* CPhysicalMaterial::getName() const
{
	return physical_mat_names.nameOf(type);
}