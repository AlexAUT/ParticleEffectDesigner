#include "particleEditorState.hpp"

#include "aw/engine/particleSystem/spawner.hpp"
#include "aw/graphics/opengl/gl.hpp"
#include "aw/util/log.hpp"
#include "aw/util/math/vector.hpp"
#include "entt/entity/helper.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"

ParticleEditorState::ParticleEditorState(aw::Engine& engine) :
    aw::State{engine.stateMachine()},
    Subscriber{engine.messageBus()},
    mEngine{engine},
    mParticleSystem{mWorld},
    mSpawner{mWorld.create()}
{
  glClearColor(0.2f, 0.8f, 0.2f, 1.0);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(engine.window().handle(), &engine.window().context());
  ImGui_ImplOpenGL3_Init("#version 430 core");

  aw::ParticleSpawner spawner;
  spawner.interval = std::normal_distribution<float>(1.f, 0.f);
  mWorld.assign<aw::ParticleSpawner>(mSpawner, spawner);
}

void ParticleEditorState::update(aw::Seconds dt)
{
  mParticleSystem.update(dt, entt::as_view(mWorld));
}

void ParticleEditorState::render()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(mEngine.window().handle());
  ImGui::NewFrame();

  ImGui::Begin("Demo Window!");
  if (ImGui::Button("Hallo Welt!")) {
    mEngine.shouldTerminate(true);
  }

  auto numParticles = mParticleSystem.particles().size();
  ImGui::Text("Active particles: %lu", numParticles);

  auto& spawner = mWorld.get<aw::ParticleSpawner>(mSpawner);
  auto mean = spawner.position[0].mean();
  if (ImGui::SliderFloat("x", &mean, 0.f, 1.f)) {
    spawner.position[0] = std::normal_distribution<float>(mean, spawner.position[0].stddev());
  }
  mean = spawner.position[1].mean();
  if (ImGui::SliderFloat("y: ", &mean, 0.f, 1.f)) {
    spawner.position[1] = std::normal_distribution<float>(mean, spawner.position[1].stddev());
  }

  auto interval = spawner.interval.mean();
  if (ImGui::SliderFloat("interval", &interval, 0.f, 10.f)) {
    spawner.interval = std::normal_distribution<float>(interval, spawner.interval.stddev());
  }

  auto amount = spawner.amount.mean();
  if (ImGui::SliderFloat("amount", &amount, 0.f, 50.f)) {
    spawner.amount = std::normal_distribution<float>(amount, spawner.amount.stddev());
  }

  auto ttl = spawner.ttl.mean();
  if (ImGui::SliderFloat("ttl", &ttl, 0.f, 10.f)) {
    spawner.ttl = std::normal_distribution<float>(ttl, spawner.ttl.stddev());
  }

  static aw::Vec3 color{0.f};

  ImGui::ColorEdit3("Color", &color.r);
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ParticleEditorState::receive(SDL_Event event)
{
  ImGui_ImplSDL2_ProcessEvent(&event);
}
