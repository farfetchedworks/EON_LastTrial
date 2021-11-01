#include "mcv_platform.h"
#include "engine.h"
#include "module_tools.h"
#include "entity/entity.h"
#include "entity/entity_picker.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "lua/module_scripting.h"
#include "modules/module_particles_editor.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_transform.h"

bool CModuleTools::start()
{
	return true;
}

void CModuleTools::renderInMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Game"))
        {
            if (ImGui::MenuItem("Exit", "ESC", false))
            {
                CApplication::get().exit();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Engine"))
        {
            if (ImGui::MenuItem("Lua Console", "", false))
            {
                EngineLua.openConsole();
            }

            if (ImGui::MenuItem("Particle Editor", "", false))
            {
                ParticlesEditor.toggle();
            }

            ImGui::EndMenu();
        }

        /*if (ImGui::BeginMenu("Show"))
        {
            
        }*/

        ImGui::EndMainMenuBar();
    }

    if (picked_entity.isValid() && picked_panel_opened)
    {
        CEntity* entity = picked_entity;

        if (ImGui::Begin("Entity picker", &picked_panel_opened)) {
            ImGui::SetNextItemOpen(true);
            entity->debugInMenu();

            TCompTransform* t = entity->get<TCompTransform>();
            if(t) t->renderGuizmo();
            ImGui::End();
        }
        else {
            ImGui::End();
        }
    }
}

void CModuleTools::update(float dt)
{
	if (debugging && PlayerInput["mouse_left"].getsPressed())
	{
		TEntityPickerContext ctx;
        bool hit = pickEntity(ctx);
        if (hit)
        {
            // ...
        }
	}
}

void CModuleTools::stop()
{
}

bool CModuleTools::pickEntity(TEntityPickerContext& ctx)
{
	CEntity* e_camera = EngineRender.getActiveCamera();
	TCompCamera* cam = e_camera->get<TCompCamera>();
	if (!cam)
		return false;

	ImVec2 mousePos = ImGui::GetMousePos();
	VEC2 pos = VEC2(mousePos.x, mousePos.y);

	TRay ray;
	ray.origin = cam->unproject(pos);
	ray.direction = normVEC3(ray.origin - e_camera->getPosition());

    bool hit = testSceneRay(ray, ctx);
    picked_panel_opened = hit;

    if (hit)
    {
        picked_entity = ctx.current_entity;
    }
    else
    {
        picked_entity = CHandle();
    }

    return hit;
}