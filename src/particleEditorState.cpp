#include "particleEditorState.hpp"

#include "aw/engine/particleSystem/spawner.hpp"
#include "aw/engine/particleSystem/spawner.serialize.hpp"
#include "aw/graphics/opengl/gl.hpp"
#include "aw/util/filesystem/fileStream.hpp"
#include "aw/util/log.hpp"
#include "aw/util/math/constants.hpp"
#include "aw/util/math/transform.hpp"
#include "aw/util/math/vector.hpp"
#include "aw/util/serialization/serialze.hpp"
#include "entt/entity/helper.hpp"
#include "fileDialog/tinyfiledialogs.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"

#include <numeric>

ParticleEditorState::ParticleEditorState(aw::Engine& engine) :
    aw::State{engine.stateMachine()},
    Subscriber{engine.messageBus()},
    mEngine{engine},
    mParticleSystem{mWorld},
    mSpawner{mWorld.create()},
    mParticleRenderer{engine.pathRegistry(), engine.window().size()}
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(engine.window().handle(), &engine.window().context());
  ImGui_ImplOpenGL3_Init("#version 430 core");

  mWorld.assign<aw::Transform>(mSpawner);
  mWorld.assign<aw::ParticleSpawner>(mSpawner);
}

void ParticleEditorState::update(aw::Seconds dt)
{
  if (mDropNextFrame) {
    mDropNextFrame = false;
    return;
  }
  mParticleSystem.update(dt, entt::as_view(mWorld));
}

void ParticleEditorState::render()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glViewport(0, 0, mEngine.window().size().x, mEngine.window().size().y);
  auto aspect = mEngine.window().size().x / static_cast<float>(mEngine.window().size().y);
  float height = 10.f;
  float heightH = height / 2.f;
  float widthH = height * aspect * 0.5f;
  auto vp = glm::orthoLH(-widthH, widthH, -heightH, heightH, -1.f, 100.f);

  auto t = mWorld.get<aw::Transform>(mSpawner);
  auto mvp = t.transform() * vp;

  mParticleRenderer.render(vp, mParticleSystem.simulationTime(), mParticleSystem.particles());

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(mEngine.window().handle());
  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();

  ImGui::Begin("Spawner Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  const auto& p = mParticleSystem.particles();
  auto numParticles =
      std::accumulate(p.begin(), p.end(), 0, [](auto sum, auto& element) { return sum + element.particles.size(); });
  ImGui::Text("Active particles: %d", numParticles);

  auto modelNormalDistribution = [this](std::normal_distribution<float>& dist, const char* name, float speed = 0.1f,
                                        float min = 0.f, float max = 100.f, float scale = 1.f, float unScale = 1.f) {
    std::array values = {dist.mean() * scale, dist.stddev() * scale};
    if (ImGui::DragFloat2(name, values.data(), speed, min, max)) {
      dist = std::normal_distribution(values[0] * unScale, values[1] * unScale);
    }
  };

  auto modelClampedNormalDistribution = [this](aw::ClampedNormalDist<float>& dist, const char* name, float speed = 0.1f,
                                               float min = 0.f, float max = 100.f, float scale = 1.f,
                                               float unScale = 1.f) {
    std::array values = {dist.min() * scale, dist.max() * scale};
    if (ImGui::DragFloat2(name, values.data(), speed, min, max)) {
      if (values[0] <= values[1]) {
        dist = aw::ClampedNormalDist(values[0] * unScale, values[1] * unScale);
      }
    }
  };

  auto& spawner = mWorld.get<aw::ParticleSpawner>(mSpawner);
  auto& spawnerTransform = mWorld.get<aw::Transform>(mSpawner);

  auto pos = spawnerTransform.position();
  if (ImGui::DragFloat3("Position", &pos.x, 0.01f, -10.f, 10.f)) {
    spawnerTransform.position(pos);
  }

  std::array mins = {spawner.position[0].min(), spawner.position[1].min(), spawner.position[2].min()};
  if (ImGui::DragFloat3("Pos offset min", mins.data(), 0.1f, -50.f, 50.f, "%.2f")) {

    if (mins[0] <= spawner.position[0].max()) {
      spawner.position[0] = aw::ClampedNormalDist(mins[0], spawner.position[0].max());
    }
    if (mins[1] <= spawner.position[1].max()) {
      spawner.position[1] = aw::ClampedNormalDist(mins[1], spawner.position[1].max());
    }
    if (mins[2] <= spawner.position[2].max()) {
      spawner.position[2] = aw::ClampedNormalDist(mins[2], spawner.position[2].max());
    }
  }
  std::array maxs = {spawner.position[0].max(), spawner.position[1].max(), spawner.position[2].max()};
  if (ImGui::DragFloat3("Pos offset max", maxs.data(), 0.01f, 0.f, 100.f, "%.2f")) {
    if (maxs[0] >= spawner.position[0].min()) {
      spawner.position[0] = aw::ClampedNormalDist(spawner.position[0].min(), maxs[0]);
    }
    if (maxs[1] >= spawner.position[1].min()) {
      spawner.position[1] = aw::ClampedNormalDist(spawner.position[1].min(), maxs[1]);
    }
    if (maxs[2] >= spawner.position[2].min()) {
      spawner.position[2] = aw::ClampedNormalDist(spawner.position[2].min(), maxs[2]);
    }
  }

  modelClampedNormalDistribution(spawner.size, "size (min/max)", 0.01f);
  modelClampedNormalDistribution(spawner.rotation, "rot (min/max)", 0.5f, 0.f, 360.f, aw::to_deg(), aw::to_rad());

  std::array velMins = {spawner.velocityDir[0].min(), spawner.velocityDir[1].min()};
  if (ImGui::DragFloat2("Vel min", velMins.data(), 0.01f, -10.f, 10.f, "%.2f")) {
    if (velMins[0] <= spawner.velocityDir[0].max()) {
      spawner.velocityDir[0] = aw::ClampedNormalDist(velMins[0], spawner.velocityDir[0].max());
    }
    if (velMins[1] <= spawner.velocityDir[1].max()) {
      spawner.velocityDir[1] = aw::ClampedNormalDist(velMins[1], spawner.velocityDir[1].max());
    }
  }
  std::array velMaxs = {spawner.velocityDir[0].max(), spawner.velocityDir[1].max()};
  if (ImGui::DragFloat2("Vel max", velMaxs.data(), 0.01f, -10.f, 10.f, "%.2f")) {
    if (velMaxs[0] >= spawner.velocityDir[0].min()) {
      spawner.velocityDir[0] = aw::ClampedNormalDist(spawner.velocityDir[0].min(), velMaxs[0]);
    }
    if (velMaxs[1] >= spawner.velocityDir[1].min()) {
      spawner.velocityDir[1] = aw::ClampedNormalDist(spawner.velocityDir[1].min(), velMaxs[1]);
    }
  }

  modelClampedNormalDistribution(spawner.amount, "amount (min/max)", 1.f, 0.f, 200.f);
  modelClampedNormalDistribution(spawner.ttl, "ttl (min/max)");
  modelClampedNormalDistribution(spawner.interval, "interval (min/max)", 0.01);

  ImGui::DragFloat("FadeIn(%)", &spawner.fadeIn, 0.01f, 0.f, 0.5f);

  ImGui::ColorEdit4("Begin", &spawner.colorGradient[0].r);
  ImGui::ColorEdit4("End", &spawner.colorGradient[1].r);

  if (ImGui::Button("New")) {
    reset();
  }
  if (ImGui::Button("Save")) {
    saveSpawner(true);
  }
  if (!mCachedSavePath.empty()) {
    ImGui::SameLine();
    if (ImGui::Button("Save as")) {
      saveSpawner(false);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Load")) {
    loadSpawner();
  }

  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ParticleEditorState::receive(SDL_Event event)
{
  ImGui_ImplSDL2_ProcessEvent(&event);
}

std::array<const char*, 1> extensions = {"*.awps"};

void ParticleEditorState::saveSpawner(bool useCachedPath)
{
  mDropNextFrame = true;

  aw::fs::path path;
  if (!useCachedPath || mCachedSavePath.empty()) {
    const auto pathPtr = tinyfd_saveFileDialog("Save particle spawner", nullptr, extensions.size(), extensions.data(),
                                               "aw particle spawner files");

    if (!pathPtr) {
      return;
    }
    mCachedSavePath = pathPtr;

    if (!mCachedSavePath.has_extension()) {
      APP_INFO("Add extension to provided file name");
      mCachedSavePath.replace_extension(".awps");
    }
  }
  APP_ERROR("Save to: {}", mCachedSavePath.c_str());

  aw::serialize::file(mCachedSavePath, mWorld.get<aw::ParticleSpawner>(mSpawner));
}

void ParticleEditorState::loadSpawner()
{
  mDropNextFrame = true;
  reset();

  auto pathPtr = tinyfd_openFileDialog("Select particle spawner", nullptr, extensions.size(), extensions.data(),
                                       "aw particle spawner files", false);

  if (!pathPtr) {
    return;
  }

  aw::fs::path path = pathPtr;
  auto particleSpawner = aw::parse::file<aw::ParticleSpawner>(path);
  mWorld.replace<aw::ParticleSpawner>(mSpawner, particleSpawner);
}

void ParticleEditorState::reset()
{
  mDropNextFrame = true;
  mWorld.replace<aw::ParticleSpawner>(mSpawner);
}
