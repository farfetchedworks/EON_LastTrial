#pragma once
#include "sol/sol.hpp"

namespace LogicManager
{
    void ldbg(sol::variadic_args args);
    void startCinematic(const std::string& curve_filename, const VEC3& target, float speed, float lerp_time);
    void startCinematicAnimation(const std::string& animation_filename, const std::string& target_name, float speed, float lerp_time, const std::string& bone_name = "");
    void startStaticCinematicAnimation(const std::string& animation_filename, float speed, float lerp_time);
    void stopCinematic(float lerp_time);
    void setCinematicCurve(const std::string& curve_filename, float curve_speed, float lerp_time);
    void setCinematicTarget(const VEC3& target, float lerp_time);
    void setCinematicTargetEntity(const std::string& entity_name, float lerp_time);
    VEC3 getEntityPosByName(const std::string& entity_name);
};
