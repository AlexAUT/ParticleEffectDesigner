#pragma once

#include "SDL_events.h"
#include "aw/engine/engine.hpp"
#include "aw/engine/particleSystem/spawner.hpp"
#include "aw/engine/particleSystem/system.hpp"
#include "aw/engine/state.hpp"
#include "aw/util/messageBus/subscriber.hpp"
#include "entt/entity/registry.hpp"

class ParticleEditorState : public aw::State, public aw::msg::Subscriber<ParticleEditorState, SDL_Event>
{
public:
  ParticleEditorState(aw::Engine& engine);

  void update(aw::Seconds dt) override;
  void render() override;

  void receive(SDL_Event event);

private:
  aw::Engine& mEngine;

  entt::registry mWorld;

  aw::ParticleSystem mParticleSystem;

  entt::entity mSpawner;
};
