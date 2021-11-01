#include "mcv_platform.h"
#include "engine.h"
#include "module_particles_editor.h"
#include "render/render_module.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_transform.h"

bool CModuleParticlesEditor::start()
{
	return true;
}

void CModuleParticlesEditor::renderInMenu()
{
    if (!opened)
        return;

    if (ImGui::Begin("Particles Editor", &opened)) {
        // ImGui::SetNextItemOpen(true);
        ImGui::End();
    }
    else {
        ImGui::End();
    }
}

void CModuleParticlesEditor::update(float dt)
{
	
}

void CModuleParticlesEditor::stop()
{

}