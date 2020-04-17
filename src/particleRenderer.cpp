#include "particleRenderer.hpp"

#include "aw/graphics/opengl/error.hpp"
#include "aw/graphics/opengl/gl.hpp"
#include "aw/graphics/opengl/shader.hpp"
#include "aw/util/color/colors.hpp"
#include "aw/util/filesystem/pathRegistry.hpp"
#include "aw/util/log.hpp"
#include "aw/util/time/time.hpp"

ParticleRenderer::ParticleRenderer(aw::PathRegistry& pathRegistry, aw::Vec2i windowSize) : mViewportSize{windowSize}
{
  GL_CHECK(glGenBuffers(1, &mVertexVbo));
  GL_CHECK(glGenBuffers(1, &mParticleVbo));

  std::array<aw::Vec2, 4> vertices{aw::Vec2{-0.5f, -0.5f}, aw::Vec2{0.5f, -0.5f}, aw::Vec2{0.5f, 0.5f},
                                   aw::Vec2{-0.5f, 0.5f}};

  GL_CHECK(glGenVertexArrays(1, &mVao));
  GL_CHECK(glBindVertexArray(mVao));

  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, mVertexVbo));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices.front()) * vertices.size(), vertices.data(), GL_STATIC_DRAW));
  GL_CHECK(glEnableVertexAttribArray(0));
  GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr));

  using Particle = aw::ParticleSystem::Particle;
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, mParticleVbo));
  // TTL/Position3D
  GL_CHECK(glEnableVertexAttribArray(1));
  GL_CHECK(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), nullptr));
  GL_CHECK(glVertexAttribDivisor(1, 1));
  // size1D
  GL_CHECK(glEnableVertexAttribArray(2));
  GL_CHECK(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
                                 reinterpret_cast<const void*>(offsetof(Particle, velocity))));
  GL_CHECK(glVertexAttribDivisor(2, 1));

  GL_CHECK(glBindVertexArray(0));

  using Type = aw::gl::shaderStage::Type;
  auto vShader = aw::gl::shaderStage::createFromFile(pathRegistry.assetPath() / "shaders/particle.vert", Type::Vertex);
  auto fShader =
      aw::gl::shaderStage::createFromFile(pathRegistry.assetPath() / "shaders/particle.frag", Type::Fragment);

  mProgram = aw::gl::shader::createProgram(vShader, fShader);

  GL_CHECK(glDeleteShader(fShader));
  GL_CHECK(glDeleteShader(vShader));

  GL_CHECK(glUseProgram(mProgram));
  mVpLoc = glGetUniformLocation(mProgram, "viewProjection");
  mSimTimeLoc = glGetUniformLocation(mProgram, "simulationTime");
  mGradientTextureLoc = glGetUniformLocation(mProgram, "colorGradient");
  GL_CHECK(glUniform1i(mGradientTextureLoc, 0));

  GL_CHECK(glUseProgram(0));

  // Generate color gradient texture
  GL_CHECK(glGenTextures(1, &mColorGradientTexture));
}

ParticleRenderer::~ParticleRenderer()
{
  GL_CHECK(glDeleteTextures(1, &mColorGradientTexture));
  GL_CHECK(glDeleteProgram(mProgram));
  GL_CHECK(glDeleteVertexArrays(1, &mVao));

  GL_CHECK(glDeleteBuffers(1, &mParticleVbo));
  GL_CHECK(glDeleteBuffers(1, &mVertexVbo));
}

void ParticleRenderer::render(const aw::Mat4& vp, aw::Seconds simulationTime,
                              const std::vector<aw::ParticleSystem::ParticleContainer>& particles)
{
  GL_CHECK(glDisable(GL_DEPTH_TEST));
  GL_CHECK(glEnable(GL_BLEND));
  GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

  GL_CHECK(glUseProgram(mProgram));
  GL_CHECK(glUniformMatrix4fv(mVpLoc, 1, GL_FALSE, &vp[0][0]));
  GL_CHECK(glUniform1f(mSimTimeLoc, simulationTime.count()));

  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_1D, mColorGradientTexture));

  GL_CHECK(glBindVertexArray(mVao));

  for (auto& pLayer : particles) {
    colorGradient(pLayer.colorGradient);

    auto& p = pLayer.particles;
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, mParticleVbo));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(p.front()) * p.size(), p.data(), GL_STREAM_DRAW));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL_CHECK(glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, p.size()));
  }

  GL_CHECK(glBindVertexArray(0));
}

void ParticleRenderer::colorGradient(Gradient gradient)
{
  if (mColorGradient == gradient) {
    return;
  }

  mColorGradient = gradient;

  GL_CHECK(glBindTexture(GL_TEXTURE_1D, mColorGradientTexture));

  GL_CHECK(glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, mColorGradient.size(), 0, GL_RGBA, GL_FLOAT, mColorGradient.data()));

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

