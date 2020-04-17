#include "particleEditorState.hpp"

#include "aw/engine/particleSystem/spawner.hpp"
#include "aw/engine/particleSystem/spawner.serialize.hpp"
#include "aw/graphics/opengl/gl.hpp"
#include "aw/util/filesystem/fileStream.hpp"
#include "aw/util/log.hpp"
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
                                        float min = 0.f, float max = 100.f) {
    std::array values = {dist.mean(), dist.stddev()};
    if (ImGui::DragFloat2(name, values.data(), speed, min, max)) {
      dist = std::normal_distribution(values[0], values[1]);
    }
  };
  auto& spawner = mWorld.get<aw::ParticleSpawner>(mSpawner);

  std::array means = {spawner.position[0].mean(), spawner.position[1].mean(), spawner.position[2].mean()};
  if (ImGui::DragFloat3("Pos", means.data(), 0.1f, -50.f, 50.f, "%.2f")) {
    spawner.position[0] = std::normal_distribution(means[0], spawner.position[0].stddev());
    spawner.position[1] = std::normal_distribution(means[1], spawner.position[1].stddev());
    spawner.position[2] = std::normal_distribution(means[2], spawner.position[2].stddev());
  }
  std::array stddevs = {spawner.position[0].stddev(), spawner.position[1].stddev(), spawner.position[2].stddev()};
  if (ImGui::DragFloat3("Std", stddevs.data(), 0.01f, 0.f, 100.f, "%.2f")) {
    spawner.position[0] = std::normal_distribution(spawner.position[0].mean(), stddevs[0]);
    spawner.position[1] = std::normal_distribution(spawner.position[1].mean(), stddevs[1]);
    spawner.position[2] = std::normal_distribution(spawner.position[2].mean(), stddevs[2]);
  }

  modelNormalDistribution(spawner.size, "size (+-", 0.01f);

  std::array velMeans = {spawner.velocityDir[0].mean(), spawner.velocityDir[1].mean()};
  if (ImGui::DragFloat2("Vel", velMeans.data(), 0.01f, 0.f, 10.f, "%.2f")) {
    spawner.velocityDir[0] = std::normal_distribution(velMeans[0], spawner.velocityDir[0].stddev());
    spawner.velocityDir[1] = std::normal_distribution(velMeans[1], spawner.velocityDir[1].stddev());
  }
  std::array velStd = {spawner.velocityDir[0].stddev(), spawner.velocityDir[1].stddev()};
  if (ImGui::DragFloat2("Std1##Vel1", velStd.data(), 0.01f, 0.f, 10.f, "%.2f")) {
    spawner.velocityDir[0] = std::normal_distribution(spawner.velocityDir[0].mean(), velStd[0]);
    spawner.velocityDir[1] = std::normal_distribution(spawner.velocityDir[1].mean(), velStd[1]);
  }

  modelNormalDistribution(spawner.amount, "amount (+-)", 1.f, 0.f, 200.f);
  modelNormalDistribution(spawner.ttl, "ttl (+-)");
  modelNormalDistribution(spawner.interval, "interval (+-)", 0.01);

  ImGui::ColorEdit4("Begin", &spawner.colorGradient[0].r);
  ImGui::ColorEdit4("End", &spawner.colorGradient[1].r);

  if (ImGui::Button("New")) {
    reset();
  }
  if (ImGui::Button("Save")) {
    saveSpawner();
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

void ParticleEditorState::saveSpawner()
{
  mDropNextFrame = true;

  auto pathPtr = tinyfd_saveFileDialog("Save particle spawner", nullptr, extensions.size(), extensions.data(),
                                       "aw particle spawner files");

  if (!pathPtr) {
    return;
  }

  aw::fs::path path = pathPtr;
  if (!path.has_extension()) {
    APP_INFO("Add extension to provided file name");
    path.replace_extension(".awps");
  }
  APP_ERROR("Save to: {}", path.c_str());

  aw::serialize::file(path, mWorld.get<aw::ParticleSpawner>(mSpawner));
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
