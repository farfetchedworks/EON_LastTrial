#pragma once

#include "entity/entity.h"
#include "comp_base.h"

class CComputeShader;
struct TCompBuffers;

class TCompCompute : public TCompBase
{
  struct TExecution
  {
    enum eCmd
    {
      RUN_COMPUTE,
      SWAP_BUFFERS,
      SWAP_TEXTURES,
    };
    eCmd                  cmd = RUN_COMPUTE;
    const CComputeShader* compute = nullptr;
    uint32_t              sizes[3];
    bool                  sets_num_instances = false;
    int                   num_executions = -1;

    bool                  is_indirect = false;
    std::string           indirect_buffer_name;
    uint32_t              indirect_buffer_offset = 0;

    std::string           swapA;
    std::string           swapB;

    void debugInMenu();
    bool getDispatchArgs(uint32_t* args);
    void load(const json& j);
    void bindArguments(TCompBuffers* c_buffers);
    void run(TCompBuffers* c_buffers);
  };

  void loadOneExecution(const json& j);

  DECL_SIBLING_ACCESS();

public:

  std::vector< TExecution > executions;

  void load(const json& j, TEntityParseContext& ctx);
  void update(float dt);
  void debugInMenu();
};