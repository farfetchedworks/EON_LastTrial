#include "mcv_platform.h"
#include "comp_skeleton_ik.h"
#include "comp_skeleton.h"
#include "handle/handle.h"
#include "skeleton/ik_handler.h"
#include "skeleton/game_core_skeleton.h"
#include "render/draw_primitives.h"
#include "skeleton/cal3d2engine.h"

DECL_OBJ_MANAGER("skel_ik", TCompSkeletonIK);

void TCompSkeletonIK::load(const json& j, TEntityParseContext& ctx) {
  bone_name = j.value("bone_name", "");
}

void TCompSkeletonIK::update(float elapsed) {

  // Access to the sibling comp skeleton component
  // where we can access the cal_model instance
  CEntity* e = CHandle(this).getOwner();
  TCompSkeleton *comp_skel = e->get<TCompSkeleton>();
  if (!comp_skel)
    return;

  if(bone_id_c == -1)
    bone_id_c = comp_skel->getBoneIdByName(bone_name);

  CalModel* model = comp_skel->model;

  assert(bone_id_c != -1);
  CalBone* bone_c = model->getSkeleton()->getBone(bone_id_c);
  
  int bone_id_b = bone_c->getCoreBone()->getParentId();
  assert(bone_id_b != -1);
  CalBone* bone_b = model->getSkeleton()->getBone(bone_id_b);
  
  int bone_id_a = bone_b->getCoreBone()->getParentId();
  assert(bone_id_a != -1);
  CalBone* bone_a = model->getSkeleton()->getBone(bone_id_a);

  // TIKHandle ik;
  ik.A = Cal2DX(bone_a->getTranslationAbsolute());
  ik.B = Cal2DX(bone_b->getTranslationAbsolute());
  ik.C = Cal2DX(bone_c->getTranslationAbsolute());
  
  // Distance from a to b, based on the skel CORE definition
  CalVector cal_ab = bone_b->getCoreBone()->getTranslationAbsolute()
                   - bone_a->getCoreBone()->getTranslationAbsolute();
  ik.AB = cal_ab.length();

  CalVector cal_bc = bone_c->getCoreBone()->getTranslationAbsolute()
                   - bone_b->getCoreBone()->getTranslationAbsolute();
  ik.BC = cal_bc.length();

  // Do a local correction by adjusting the height of the foot
  ik.C.y += delta_y_over_c;

  // The initial plane formed by A,B,C
  ik.normal = (ik.C - ik.A).Cross(ik.B - ik.A);
  ik.normal.Normalize();
  ik.solveB();

  if (amount == 0.f)
    return;

  //// Correct A to point to B
  TBoneCorrection bc;
  bc.bone_id = bone_id_a;
  bc.amount = 1.0f;
  bc.local_axis_to_correct = AB_dir;
  bc.apply(model->getSkeleton(), ik.B, amount);

  // Correct B to point to C
  bc.bone_id = bone_id_b;
  bc.amount = 1.0f;
  bc.local_axis_to_correct = AB_dir;
  bc.apply(model->getSkeleton(), ik.C, amount);
}

void TCompSkeletonIK::debugInMenu() {
  ImGui::DragFloat("Amount", &amount, 0.01f, 0, 1.f);
  ImGui::DragFloat("DeltaY", &delta_y_over_c, 0.01f, -0.4f, 0.4f);
  ImGui::DragFloat3("ABDir", &AB_dir.x, 0.01f, -1.0f, 1.0f);
}

void TCompSkeletonIK::renderDebug() {
  drawText3D(ik.A, Colors::Red, "A");
  drawText3D(ik.B, Colors::Red, "B");
  drawText3D(ik.C, Colors::Red, "C");
}
