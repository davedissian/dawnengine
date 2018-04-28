/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2017 (git@dga.me.uk)
 */
#include "Common.h"
#include "Entity.h"

namespace dw {
Entity::Entity(Context* context, Ontology::EntityManager& entity_manager, EntityId id,
               EntityType type)
    : Object{context},
      id_{id},
      type_{type},
      internal_entity_mgr_{entity_manager},
      internal_entity_id_{entity_manager.createEntity("").getID()},
      transform_{nullptr} {
    entity().addComponent<OntologyMetadata>(this);
}

EntityId Entity::id() const {
    return id_;
}

EntityType Entity::typeId() const {
    return type_;
}

Transform* Entity::transform() const {
    return component<Transform>();
}
}  // namespace dw
