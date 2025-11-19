#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "../include/raylib.h"

#define GRID_WIDTH 1024
#define GRID_HEIGHT 1024
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 720
#define INITIAL_CHANCE 30

#define SWAP(a, b) { \
  RenderTexture2D *temp = a; \
  a = b; \
  b = temp; \
}

typedef struct VelocityParams {
  float strength;
  float maxSpeed;
  float encodeRange;
  float influenceRadius;
} VelocityParams;

static Rectangle RenderTextureSource(const RenderTexture2D *target) {
  return (Rectangle){
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)target->texture.width,
      .height = -(float)target->texture.height,
  };
}

static void ResetBuffer(RenderTexture2D target) {
  BeginTextureMode(target);
  ClearBackground(BLACK);
  int ymid = GRID_HEIGHT / 2.0f;
  int xmid = GRID_WIDTH / 2.0f;

  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
      DrawPixel(x + xmid, y + ymid, RAYWHITE);
    }
  }

  EndTextureMode();
}

static void CopyBuffer(RenderTexture2D dst, RenderTexture2D src) {
  BeginTextureMode(dst);
  ClearBackground(BLACK);
  DrawTextureRec(src.texture, RenderTextureSource(&src), (Vector2){0.0f, 0.0f}, WHITE);
  EndTextureMode();
}

static uint8_t EncodeVelocityChannel(float value, const VelocityParams *params) {
  float range = fmaxf(params->encodeRange, 1e-8f);
  float clamped = fmaxf(fminf(value, range), -range);
  float normalized = (clamped / range) * 0.5f + 0.5f;
  normalized = fmaxf(fminf(normalized, 1.0f), 0.0f);
  return (uint8_t)roundf(normalized * 255.0f);
}

static Color VelocityColor(float value, const VelocityParams *params) {
  uint8_t c = EncodeVelocityChannel(value, params);
  return (Color){c, c, c, 255};
}

static Vector2 InitialVelocityField(int x, int y, const VelocityParams *params) {
  float cx = GRID_WIDTH * 0.5f;
  float cy = GRID_HEIGHT * 0.5f;
  float px = (float)x + 0.5f - cx;
  float py = (float)y + 0.5f - cy;
  float dist = sqrtf(px * px + py * py) + 1.0f;
  float strength = params->strength;
  Vector2 vel = {-py, px};
  vel.x = (vel.x / dist) * strength;
  vel.y = (vel.y / dist) * strength;
  float maxSpeed = fmaxf(params->maxSpeed, 1e-8f);
  vel.x = fmaxf(fminf(vel.x, maxSpeed), -maxSpeed);
  vel.y = fmaxf(fminf(vel.y, maxSpeed), -maxSpeed);
  return vel;
}

static void WriteVelocityComponent(RenderTexture2D target, bool horizontal,
                                   const VelocityParams *params) {
  BeginTextureMode(target);
  Color zeroColor = VelocityColor(0.0f, params);
  ClearBackground(zeroColor);
  float influenceRadius = params->influenceRadius;
  float cx = GRID_WIDTH * 0.5f;
  float cy = GRID_HEIGHT * 0.5f;
  for (int y = 0; y < GRID_HEIGHT; y++) {
    for (int x = 0; x < GRID_WIDTH; x++) {
      float dx = (float)x + 0.5f - cx;
      float dy = (float)y + 0.5f - cy;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist <= influenceRadius) {
        Vector2 vel = InitialVelocityField(x, y, params);
        float value = horizontal ? vel.x : vel.y;
        DrawPixel(x, y, VelocityColor(value, params));
      } else {
        DrawPixel(x, y, zeroColor);
      }
    }
  }
  EndTextureMode();
}

static void ResetVelocityBuffers(RenderTexture2D uTarget, RenderTexture2D vTarget,
                                 const VelocityParams *params) {
  WriteVelocityComponent(uTarget, true, params);
  WriteVelocityComponent(vTarget, false, params);
}

int main(void) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Shader Fluid Sim");
  SetTargetFPS(60);

  SetRandomSeed((unsigned int)time(NULL));

  RenderTexture2D buffers[2] = {
      LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT),
      LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT),
  };

  ResetBuffer(buffers[0]);
  CopyBuffer(buffers[1], buffers[0]);

  RenderTexture2D uVelocityBuffer = LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT);
  RenderTexture2D vVelocityBuffer = LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT);

  VelocityParams velocityParams = {
      .strength = 0.001f,
      .maxSpeed = 0.03f,
      .encodeRange = 0.0001f,
      .influenceRadius = 20.0f,
  };
  velocityParams.maxSpeed = fmaxf(velocityParams.maxSpeed, 1e-8f);
  velocityParams.encodeRange = fmaxf(velocityParams.encodeRange, velocityParams.maxSpeed);
  ResetVelocityBuffers(uVelocityBuffer, vVelocityBuffer, &velocityParams);

  Shader diffuse_shader = LoadShader(NULL, "src/shaders/diffuse.frag");
  int resolutionLoc = GetShaderLocation(diffuse_shader, "resolution");
  int pausedLoc = GetShaderLocation(diffuse_shader, "paused");
  int gridSizeLoc = GetShaderLocation(diffuse_shader, "gridSize");
  int dtLoc = GetShaderLocation(diffuse_shader, "dt");
  int diffLoc = GetShaderLocation(diffuse_shader, "diff");
  int sourceRadiusLoc = GetShaderLocation(diffuse_shader, "sourceRadius");
  int sourceStrengthLoc = GetShaderLocation(diffuse_shader, "sourceStrength");

  Shader advect_shader = LoadShader(NULL, "src/shaders/advect.frag");
  int advectResolutionLoc = GetShaderLocation(advect_shader, "resolution");
  int advectGridSizeLoc = GetShaderLocation(advect_shader, "gridSize");
  int advectDtLoc = GetShaderLocation(advect_shader, "dt");
  int advectPausedLoc = GetShaderLocation(advect_shader, "paused");
  int advectUVelLoc = GetShaderLocation(advect_shader, "uVelBuffer");
  int advectVVelLoc = GetShaderLocation(advect_shader, "vVelBuffer");
  int advectVelocityRangeLoc =
      GetShaderLocation(advect_shader, "velocityRange");

  Vector2 resolution = {(float)GRID_WIDTH, (float)GRID_HEIGHT};
  int gridSize = GRID_WIDTH;
  float dt = 1.0f / 60.0f;
  float diff = 0.1f;
  float sourceRadius = 1.0f;
  float sourceStrength = 1.0f;
  SetShaderValue(diffuse_shader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(diffuse_shader, gridSizeLoc, &gridSize, SHADER_UNIFORM_INT);
  SetShaderValue(diffuse_shader, dtLoc, &dt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(diffuse_shader, diffLoc, &diff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(diffuse_shader, sourceRadiusLoc, &sourceRadius, SHADER_UNIFORM_FLOAT);
  SetShaderValue(diffuse_shader, sourceStrengthLoc, &sourceStrength, SHADER_UNIFORM_FLOAT);

  SetShaderValue(advect_shader, advectResolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(advect_shader, advectGridSizeLoc, &gridSize, SHADER_UNIFORM_INT);
  SetShaderValue(advect_shader, advectDtLoc, &dt, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(advect_shader, advectUVelLoc, uVelocityBuffer.texture);
  SetShaderValueTexture(advect_shader, advectVVelLoc, vVelocityBuffer.texture);
  SetShaderValue(advect_shader, advectVelocityRangeLoc, &velocityParams.encodeRange,
                 SHADER_UNIFORM_FLOAT);

  bool paused = false;
  RenderTexture2D *readTarget = &buffers[0];
  RenderTexture2D *writeTarget = &buffers[1];

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) {
      paused = !paused;
    }
    if (IsKeyPressed(KEY_R)) {
      ResetBuffer(*readTarget);
      CopyBuffer(*writeTarget, *readTarget);
      ResetVelocityBuffers(uVelocityBuffer, vVelocityBuffer, &velocityParams);
    }
    bool stepOnce = IsKeyPressed(KEY_RIGHT);

    int shaderPaused = (paused && !stepOnce) ? 1 : 0;
    SetShaderValue(diffuse_shader, pausedLoc, &shaderPaused, SHADER_UNIFORM_INT);
    SetShaderValue(advect_shader, advectPausedLoc, &shaderPaused, SHADER_UNIFORM_INT);

    BeginTextureMode(*writeTarget);
    ClearBackground(BLACK);
    BeginShaderMode(advect_shader);
    DrawTextureRec(readTarget->texture, RenderTextureSource(readTarget), (Vector2){0.0f, 0.0f}, WHITE);
    EndShaderMode();
    EndTextureMode();
    SWAP(readTarget, writeTarget);

    for (int k = 0; k < 20; k++) {
      BeginTextureMode(*writeTarget);
      ClearBackground(BLACK);
      BeginShaderMode(diffuse_shader);
      DrawTextureRec(readTarget->texture, RenderTextureSource(readTarget), (Vector2){0.0f, 0.0f}, WHITE);
      EndShaderMode();
      EndTextureMode();
      SWAP(readTarget, writeTarget);
    }

    BeginDrawing();
    ClearBackground((Color){20, 20, 20, 255});

    float scale = fminf((float)WINDOW_WIDTH / GRID_WIDTH, (float)WINDOW_HEIGHT / GRID_HEIGHT);
    float renderWidth = GRID_WIDTH * scale;
    float renderHeight = GRID_HEIGHT * scale;
    Rectangle source = RenderTextureSource(readTarget);
    Rectangle dest = {(WINDOW_WIDTH - renderWidth) * 0.5f,
                      (WINDOW_HEIGHT - renderHeight) * 0.5f,
                      renderWidth,
                      renderHeight};

    DrawTexturePro(readTarget->texture, source, dest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

    DrawText("Space: Pause/Resume", 24, WINDOW_HEIGHT - 96, 20, RAYWHITE);
    DrawText("->: Step once", 24, WINDOW_HEIGHT - 72, 20, RAYWHITE);
    DrawText("R: Reset", 24, WINDOW_HEIGHT - 48, 20, RAYWHITE);
    DrawText(paused ? "PAUSED" : "RUNNING", 24, WINDOW_HEIGHT - 120, 20,
             paused ? RED : GREEN);

    DrawFPS(10, 10);
    EndDrawing();
  }

  UnloadShader(diffuse_shader);
  UnloadShader(advect_shader);
  UnloadRenderTexture(buffers[0]);
  UnloadRenderTexture(buffers[1]);
  UnloadRenderTexture(uVelocityBuffer);
  UnloadRenderTexture(vVelocityBuffer);
  CloseWindow();

  return 0;
}
