#include "mcv_platform.h"
#include "entity.h"
#include "entity_parser.h"
#include "components/common/comp_parent.h"
#include "utils/resource_json.h"
#include "components/messages.h"
#include "components/common/comp_name.h"

TEntityParseContext::TEntityParseContext(TEntityParseContext& another_ctx, const CTransform& delta_transform) {
  parent = &another_ctx;
  recursion_level = another_ctx.recursion_level + 1;
  entity_starting_the_parse = another_ctx.entity_starting_the_parse;
  root_transform = another_ctx.root_transform.combinedWith(delta_transform);
  //VEC3 p = root_transform.getPosition(); float y, pitch; root_transform.getYawPitchRoll(&y, &pitch);
  //dbg("New root transform is Pos:%f %f %f Yaw: %f\n", p.x, p.y, p.z, rad2deg(y));
}

// Find in the current list of entities created, the first entity matching
// the given name
CHandle TEntityParseContext::findEntityByName(const std::string& name) const {

  // Search linearly in the list of entity currently loaded
  for (auto h : entities_loaded) {
    CEntity* e = h;
    if (e->getName() == name)
      return h;
  }

  // Delegate it to my parent
  if (parent)
    return parent->findEntityByName(name);

  // Use the global entity 'getEntityByName' if all other context aware options fail
  return getEntityByName(name);
}

CHandle spawn(const std::string& filename, CTransform root, TEntityParseContext& ctx) {
  ctx.root_transform = root;
  if (!parseScene(filename, ctx))
    return CHandle();
  assert(!ctx.entities_loaded.empty());

  //for (auto h : ctx.entities_loaded) {
  //    CEntity* e = h;
  //    if (!e) continue;
  //    // Don't spawn things with same name
  //    if (getEntityByName(e->getName()).isValid()) {
  //        std::string new_name = e->getName();
  //        new_name += std::to_string(Random::unit());
  //        e->setName(new_name.c_str());
  //    }
  //}

  return ctx.entities_loaded[0];
}

CHandle spawn(const std::string& filename, CTransform root) {
    TEntityParseContext ctx;
    return spawn(filename, root, ctx);
}

bool parseScene(const std::string& filename, TEntityParseContext& ctx) {
  std::string title = std::string("Parse " + filename);
  PROFILE_FUNCTION(title);
  TFileContext file_ctx(filename);

  ctx.filename = filename;

  // Instead of loading the json, use the already loaded & parsed json resource
  const CJson* j_res = Resources.get(filename)->as<CJson>();
  const json& jscene = j_res->getJson();

  assert(jscene.is_array());

  // For each item in the scene as array
  for (int i = 0; i < jscene.size(); ++i) {

    // get access to the i-th tem
    const json& j_item = jscene[i];
    assert(j_item.is_object());

    if (j_item.count("entity")) {
      PROFILE_FUNCTION("Entity");
      const json& j_entity = j_item["entity"];

      CHandle h_entity;

      // Do we have a prefab key in the json?
      if (j_entity.count("prefab")) {

        // get the name of the prefab
        std::string prefab_src = j_entity["prefab"];
        assert(!prefab_src.empty());

        // Get the delta transform where we should instantiate this prefab
        CTransform delta_transform;
        if (j_entity.count("transform"))
          delta_transform.fromJson(j_entity["transform"]);

        TEntityParseContext prefab_ctx(ctx, delta_transform);
        prefab_ctx.parsing_prefab = true;
        if (!parseScene(prefab_src, prefab_ctx))
          return false;

        // Append all the entities created in the prefab to the 'all entities loaded' array
        ctx.all_entities_loaded.insert(
          ctx.all_entities_loaded.end(),
          prefab_ctx.all_entities_loaded.begin(),
          prefab_ctx.all_entities_loaded.end()
        );

        // Get the first entity loaded, because we need to customize it a bit
        h_entity = prefab_ctx.entities_loaded[0];


        // Convert from handle to pointer obj
        CEntity* entity = h_entity;

        std::string old_name = entity->getName();

        json j_entity_without_transform = j_entity;
        j_entity_without_transform.erase("transform");
        j_entity_without_transform.erase("prefab");

        j_entity_without_transform["from_prefab"] = entity->getName();

        entity->load(j_entity_without_transform, ctx);

        // TCompName::all_names.erase(old_name);
        TCompName::all_prefabs[old_name].push_back(entity);
      }
      else {

        // Create a handle to contain all the components associated
        h_entity.create<CEntity>();

        // Convert from handle to pointer obj
        CEntity* entity = h_entity;

        entity->load(j_entity, ctx);

        ctx.all_entities_loaded.push_back(h_entity);
      }

      // Register that entity was loaded here
      ctx.entities_loaded.push_back(h_entity);
    }

  }

  //if (ctx.entities_loaded.size() > 1) {

  //  // The first entity becomes the parent of all the other
  //  CHandle h_root_of_group = ctx.entities_loaded[0];
  //  CEntity* e_root = h_root_of_group;
  //  assert(e_root);

  //  // Create a new instance of the TCompGroup
  //  CHandle h_parent = getObjectManager<TCompParent>()->createHandle();
  //  e_root->set(h_parent);

  //  // Add the rest of the entities as child
  //  TCompParent* c_parent = h_parent;
  //  for (size_t i = 1; i < ctx.entities_loaded.size(); ++i)
  //    c_parent->addChild(ctx.entities_loaded[i]);

  //}

  // We have finish parsing all the components of the entity
  if (!ctx.parsing_prefab) {
    for(auto h : ctx.all_entities_loaded)
      h.onEntityCreated();

    /*TMsgAllEntitiesCreated msg;
    for (auto h : ctx.entities_loaded)
        h.sendMsg(msg);*/
  }

  return true;
}

bool parseSceneWithTag(const std::string& filename, const std::string& tag, TEntityParseContext& ctx, std::function<bool(const std::string&)> fn) {

    ctx.filename = filename;

    // Instead of loading the json, use the already loaded & parsed json resource
    const CJson* j_res = Resources.get(filename)->as<CJson>();
    const json& jscene = j_res->getJson();

    assert(jscene.is_array());

    // For each item in the scene as array
    for (int i = 0; i < jscene.size(); ++i) {

        // get access to the i-th tem
        const json& j_item = jscene[i];
        assert(j_item.is_object());

        if (j_item.count("entity")) {
            const json& j_entity = j_item["entity"];

            CHandle h_entity;

            if (j_entity.count("tags")) {

                const json& j_tags = j_entity["tags"];
                assert(j_tags.is_array());

                bool found = false;

                for (auto t : j_tags) {
                    if (t == tag) {
                        found = true;
                        break;
                    }
                }

                // no specific tag -> don't parse
                if (!found)
                    continue;
                else if (!fn(j_entity["name"]))
                    continue;
            }
            // no tags -> tags can be in prefab
            // no tags and no prefab -> no parsing
            else if(!j_entity.count("prefab"))
            {
                continue;
            }

            // Do we have a prefab key in the json?
            if (j_entity.count("prefab")) {

                // get the name of the prefab
                std::string prefab_src = j_entity["prefab"];
                assert(!prefab_src.empty());

                // Get the delta transform where we should instantiate this prefab
                CTransform delta_transform;
                if (j_entity.count("transform"))
                    delta_transform.fromJson(j_entity["transform"]);

                TEntityParseContext prefab_ctx(ctx, delta_transform);
                prefab_ctx.parsing_prefab = true;
                if (!parseSceneWithTag(prefab_src, tag, prefab_ctx, fn))
                    return false;

                // Append all the entities created in the prefab to the 'all entities loaded' array
                ctx.all_entities_loaded.insert(
                    ctx.all_entities_loaded.end(),
                    prefab_ctx.all_entities_loaded.begin(),
                    prefab_ctx.all_entities_loaded.end()
                );

                // since we are filtering by tag, entities_loaded could be empty
                if (prefab_ctx.entities_loaded.size()) {

                    // Get the first entity loaded, because we need to customize it a bit
                    h_entity = prefab_ctx.entities_loaded[0];

                    // Convert from handle to pointer obj
                    CEntity* entity = h_entity;

                    json j_entity_without_transform = j_entity;
                    j_entity_without_transform.erase("transform");

                    entity->load(j_entity_without_transform, ctx);
                }
            }
            else {

                // Create a handle to contain all the components associated
                h_entity.create<CEntity>();

                // Convert from handle to pointer obj
                CEntity* entity = h_entity;

                entity->load(j_entity, ctx);

                ctx.all_entities_loaded.push_back(h_entity);
            }

            // Register that entity was loaded here
            if(h_entity.isValid())
                ctx.entities_loaded.push_back(h_entity);
        }

    }

    //if (ctx.entities_loaded.size() > 1) {

    //    // The first entity becomes the parent of all the other
    //    CHandle h_root_of_group = ctx.entities_loaded[0];
    //    CEntity* e_root = h_root_of_group;
    //    assert(e_root);

    //    // Create a new instance of the TCompGroup
    //    CHandle h_parent = getObjectManager<TCompParent>()->createHandle();
    //    e_root->set(h_parent);

    //    // Add the rest of the entities as child
    //    TCompParent* c_parent = h_parent;
    //    for (size_t i = 1; i < ctx.entities_loaded.size(); ++i)
    //        c_parent->addChild(ctx.entities_loaded[i]);

    //}

    // We have finish parsing all the components of the entity
    if (!ctx.parsing_prefab) {
        for (auto h : ctx.all_entities_loaded)
            h.onEntityCreated();
    }

    return true;
}


bool destroyScene(const std::string& filename)
{
  json jscene = loadJson(filename);
  assert(jscene.is_array());

  // For each item in the scene as array
  for (int i = 0; i < jscene.size(); ++i) {

    // get access to the i-th tem
    const json& j_item = jscene[i];
    assert(j_item.is_object());

    if (j_item.count("entity")) {
      const json& j_entity = j_item["entity"];
      std::string j_name = j_entity["name"];
      CEntity* e_entity = getEntityByName(j_name);
      e_entity->destroy();
    }
  }
  return true;
}