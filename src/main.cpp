#include "aw/engine/engine.hpp"
#include "particleEditorState.hpp"

#include "aw/util/log.hpp"

auto main(int argc, char** argv) -> int
{
  aw::Engine engine(argc, argv, "awParticleEditor");

  engine.stateMachine().pushState(std::make_unique<ParticleEditorState>(engine));

  engine.run();

  return 0;
}
