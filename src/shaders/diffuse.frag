#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform int paused;
uniform int gridSize;
uniform float dt;
uniform float diff;
uniform float sourceRadius;
uniform float sourceStrength;

out vec4 finalColor;

float sampleState(vec2 offset) {
  vec2 uv = fragTexCoord + offset / resolution;
  uv = fract(uv);
  return step(0.5, texture(texture0, uv).r);
}

float sample(vec2 offset) {
  vec2 uv = fragTexCoord + offset / resolution;
  uv = fract(uv);
  return texture(texture0, uv).x;
}

void main() {
  float N = float(gridSize);
  float a = dt * diff * N * N;

  vec2 gridCoord = fragTexCoord * N;
  bool isInterior = gridCoord.x > 0.5 && gridCoord.x < (N - 0.5) &&
                    gridCoord.y > 0.5 && gridCoord.y < (N - 0.5);

  if (!isInterior) {
    finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }

  float mid   = sample(vec2( 0,  0));
  float left  = sample(vec2(-1,  0));
  float right = sample(vec2( 1,  0));
  float up    = sample(vec2( 0,  1));
  float down  = sample(vec2( 0, -1));

  if (paused == 1) {
    finalColor = vec4(vec3(mid), 1.0);
    return;
  }

  // Jacobi iteration
  float next = (mid + a * (left + right + up + down)) / (1.0 + 4.0 * a);

  vec2 center = vec2(0.5 * N);
  float dist = length(gridCoord - center);
  float sourceMask = 1.0 - smoothstep(sourceRadius, sourceRadius + 1.0, dist);
  next += sourceMask * sourceStrength;

  finalColor = vec4(vec3(next), 1.0);
}

