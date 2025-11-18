#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "../include/raylib.h"

#define GRID_WIDTH 256
#define GRID_HEIGHT 256
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 720
#define INITIAL_CHANCE 30

#define SWAP(a, b) { \
  RenderTexture2D *temp = a; \
  a = b; \
  b = temp; \
}

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

  for (int y = -5; y <= 5; y++) {
    for (int x = -5; x <= 5; x++) {
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

int main(void) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Shader Game of Life");
  SetTargetFPS(60);

  SetRandomSeed((unsigned int)time(NULL));

  RenderTexture2D buffers[2] = {
      LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT),
      LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT),
  };

  ResetBuffer(buffers[0]);
  CopyBuffer(buffers[1], buffers[0]);

  Shader golShader = LoadShader(NULL, "src/shaders/diffuse.frag");
  int resolutionLoc = GetShaderLocation(golShader, "resolution");
  int pausedLoc = GetShaderLocation(golShader, "paused");
  int gridSizeLoc = GetShaderLocation(golShader, "gridSize");
  int dtLoc = GetShaderLocation(golShader, "dt");
  int diffLoc = GetShaderLocation(golShader, "diff");

  Vector2 resolution = {(float)GRID_WIDTH, (float)GRID_HEIGHT};
  int gridSize = GRID_WIDTH;
  float dt = 1.0f / 60.0f;
  float diff = 0.00050f;
  SetShaderValue(golShader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(golShader, gridSizeLoc, &gridSize, SHADER_UNIFORM_INT);
  SetShaderValue(golShader, dtLoc, &dt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(golShader, diffLoc, &diff, SHADER_UNIFORM_FLOAT);

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
    }
    bool stepOnce = IsKeyPressed(KEY_N);

    int shaderPaused = (paused && !stepOnce) ? 1 : 0;
    SetShaderValue(golShader, pausedLoc, &shaderPaused, SHADER_UNIFORM_INT);

    for (int i = 0; i < 20; i++) {
      BeginTextureMode(*writeTarget);
      ClearBackground(BLACK);
      BeginShaderMode(golShader);
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
    DrawText("N: Step once", 24, WINDOW_HEIGHT - 72, 20, RAYWHITE);
    DrawText("R: Reset", 24, WINDOW_HEIGHT - 48, 20, RAYWHITE);
    DrawText(paused ? "PAUSED" : "RUNNING", 24, WINDOW_HEIGHT - 120, 20,
             paused ? RED : GREEN);

    DrawFPS(10, 10);
    EndDrawing();
  }

  UnloadShader(golShader);
  UnloadRenderTexture(buffers[0]);
  UnloadRenderTexture(buffers[1]);
  CloseWindow();

  return 0;
}
