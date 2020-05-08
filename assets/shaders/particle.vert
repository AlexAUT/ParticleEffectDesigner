layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec4 particlePosSize;
layout(location = 2) in vec4 velocityAliveUntilAliveFor;
layout(location = 3) in float rotation;

uniform mat4 viewProjection;
uniform float simulationTime;

uniform sampler1D colorGradient;

out flat float ttl;
out vec4 ttlColor;

void main()
{
  //Calculate ttl stuff
  ttl = (velocityAliveUntilAliveFor.z - simulationTime);
  float ttlPercent = ttl * (1.0 / velocityAliveUntilAliveFor.w);

  ttlColor = texture(colorGradient, 1.0 - ttlPercent);

  float fullLifeDuration = velocityAliveUntilAliveFor.w;
  float lifePassed = fullLifeDuration - ttl;

  //Calculate position
  vec2 movement = lifePassed * velocityAliveUntilAliveFor.xy;

  float size = particlePosSize.w;
  size = size * 0.5 * ttlPercent + size * 0.5;
  vec4 particleCenter = viewProjection * vec4(particlePosSize.xyz + vec3(movement, 0.0), 1.0);
  vec2 vPos = vertexPosition * vec2(size, size);

  float cosRot = cos(rotation);
  float sinRot = sin(rotation);
  vPos = vec2(vPos.x * cosRot - vPos.y * sinRot, vPos.x * sinRot + vPos.y * cosRot);

  vec4 vertexPositionScreenspace = viewProjection * vec4(vPos.x, vPos.y, 0.f, 1.0);

  vec4 sPos = vertexPositionScreenspace + particleCenter;
  gl_Position = sPos;
}
