// Fragment shader implementation of the Jos Stam advection step.
// d       -> write target (current density buffer)
// d0      -> read source (previous density buffer, bound as texture0)
// u / v   -> velocity buffers supplying horizontal/vertical components

#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;      // d0
uniform sampler2D uVelBuffer;    // u
uniform sampler2D vVelBuffer;    // v
uniform vec2 resolution;
uniform int gridSize;
uniform float dt;
uniform int paused;
uniform float velocityRange;

out vec4 finalColor;

float sampleDensityAt(vec2 index, float N) {
  vec2 uv = (index - vec2(0.5)) / N;
  uv = clamp(uv, vec2(0.0), vec2(1.0));
  return texture(texture0, uv).x;
}

float decodeVelocity(float encoded) {
  return (encoded * 2.0 - 1.0) * velocityRange;
}

void main() {
  float N = float(gridSize);
  vec2 cell = fragTexCoord * N;
  bool isInterior = cell.x > 0.5 && cell.x < (N - 0.5) &&
                    cell.y > 0.5 && cell.y < (N - 0.5);

  if (!isInterior) {
    finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }

  if (paused == 1) {
    float current = texture(texture0, fragTexCoord).x;
    finalColor = vec4(vec3(current), 1.0);
    return;
  }

  float dt0 = dt * N;
  float u = decodeVelocity(texture(uVelBuffer, fragTexCoord).x);
  float v = decodeVelocity(texture(vVelBuffer, fragTexCoord).x);

  vec2 coord = cell + vec2(0.5);
  vec2 prev = coord - dt0 * vec2(u, v);
  prev = clamp(prev, vec2(1.0), vec2(N));

  vec2 i0 = floor(prev);
  vec2 i1 = i0 + vec2(1.0);
  i1 = clamp(i1, vec2(1.0), vec2(N));

  vec2 frac = prev - i0;
  float s1 = frac.x;
  float s0 = 1.0 - s1;
  float t1 = frac.y;
  float t0 = 1.0 - t1;

  float d00 = sampleDensityAt(vec2(i0.x, i0.y), N);
  float d10 = sampleDensityAt(vec2(i1.x, i0.y), N);
  float d01 = sampleDensityAt(vec2(i0.x, i1.y), N);
  float d11 = sampleDensityAt(vec2(i1.x, i1.y), N);

  float density = s0 * (t0 * d00 + t1 * d01)
                + s1 * (t0 * d10 + t1 * d11);

  finalColor = vec4(vec3(density), 1.0);
}

