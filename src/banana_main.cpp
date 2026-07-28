#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <vector>

namespace {

constexpr int kWindowWidth = 600;
constexpr int kWindowHeight = 645;
constexpr Color kInk = {20, 16, 12, 255};
void UseApplicationResources() {
    namespace fs = std::filesystem;
    const fs::path executableDirectory = GetApplicationDirectory();
    const fs::path bundledResources = executableDirectory / ".." / "Resources";
    const fs::path resourceDirectory = fs::is_directory(bundledResources / "assets")
        ? bundledResources
        : executableDirectory;
    ChangeDirectory(resourceDirectory.string().c_str());
}

Rectangle FitTexture(Rectangle destination) {
    return {destination.x, destination.y, destination.width, destination.height};
}

bool ButtonHit(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) &&
           IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

void DrawImageButton(Texture2D texture, Rectangle bounds) {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    const Color tint = hovered ? ColorBrightness(WHITE, 0.12f) : WHITE;
    DrawTexturePro(texture, {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                   FitTexture(bounds), {0, 0}, 0.0f, tint);
}

Mesh CopyDynamicMesh(const Mesh& source) {
    Mesh copy{};
    copy.vertexCount = source.vertexCount;
    copy.triangleCount = source.triangleCount;
    const size_t vertexBytes = static_cast<size_t>(source.vertexCount) * 3 * sizeof(float);
    copy.vertices = static_cast<float*>(MemAlloc(vertexBytes));
    std::memcpy(copy.vertices, source.vertices, vertexBytes);

    if (source.texcoords != nullptr) {
        const size_t texcoordBytes = static_cast<size_t>(source.vertexCount) * 2 * sizeof(float);
        copy.texcoords = static_cast<float*>(MemAlloc(texcoordBytes));
        std::memcpy(copy.texcoords, source.texcoords, texcoordBytes);
    }
    if (source.normals != nullptr) {
        copy.normals = static_cast<float*>(MemAlloc(vertexBytes));
        std::memcpy(copy.normals, source.normals, vertexBytes);
    }
    if (source.colors != nullptr) {
        const size_t colorBytes = static_cast<size_t>(source.vertexCount) * 4 * sizeof(unsigned char);
        copy.colors = static_cast<unsigned char*>(MemAlloc(colorBytes));
        std::memcpy(copy.colors, source.colors, colorBytes);
    }
    if (source.indices != nullptr) {
        const size_t indexBytes = static_cast<size_t>(source.triangleCount) * 3 * sizeof(unsigned short);
        copy.indices = static_cast<unsigned short*>(MemAlloc(indexBytes));
        std::memcpy(copy.indices, source.indices, indexBytes);
    }
    UploadMesh(&copy, true);
    return copy;
}

}  // namespace

int main() {
    UseApplicationResources();
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE |
                   FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT);
    InitWindow(kWindowWidth, kWindowHeight, "BANANA DANCE");
    SetTargetFPS(60);

    Texture2D cardboard = LoadTexture("assets/banana/ui/cardboard.png");
    Texture2D minimize = LoadTexture("assets/banana/ui/minimize.png");
    Texture2D maximize = LoadTexture("assets/banana/ui/maximize.png");
    Texture2D close = LoadTexture("assets/banana/ui/close-red.png");
    Model baseBanana = LoadModel("assets/banana/models/banana_base.glb");
    Model danceBanana = LoadModel("assets/banana/models/banana_dance.glb");
    if (baseBanana.meshCount == 0 || danceBanana.meshCount == 0 ||
        baseBanana.meshes[0].vertexCount != danceBanana.meshes[0].vertexCount) {
        CloseWindow();
        return 1;
    }
    Model morphBanana = LoadModelFromMesh(CopyDynamicMesh(baseBanana.meshes[0]));
    morphBanana.materials[0].maps[MATERIAL_MAP_DIFFUSE].color =
        baseBanana.materials[0].maps[MATERIAL_MAP_DIFFUSE].color;
    const size_t morphVertexCount = static_cast<size_t>(baseBanana.meshes[0].vertexCount) * 3;
    std::vector<float> morphVertices(morphVertexCount);
    RenderTexture2D outlineLayer = LoadRenderTexture(kWindowWidth, kWindowHeight);
    RenderTexture2D bananaLayer = LoadRenderTexture(kWindowWidth, kWindowHeight);

    Camera3D camera{};
    camera.position = {4.7f, 2.9f, 7.2f};
    camera.target = {0.0f, 0.55f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 38.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool maximized = false;
    float danceTime = 0.0f;
    while (!WindowShouldClose()) {
        danceTime += GetFrameTime();
        // Six frames at 60 FPS. The form hits its full "dance" shape exactly
        // 0.1 seconds after leaving the base shape, then returns in 0.1 seconds.
        constexpr float kMorphDuration = 0.1f;
        const float cycleTime = fmodf(danceTime, kMorphDuration * 2.0f);
        const float phase = cycleTime <= kMorphDuration
            ? cycleTime / kMorphDuration
            : 1.0f - (cycleTime - kMorphDuration) / kMorphDuration;
        const float turn = danceTime * 152.0f;
        const float hop = sinf(danceTime * 5.0f) * 0.16f + phase * 0.12f;

        for (size_t i = 0; i < morphVertexCount; ++i) {
            morphVertices[i] = Lerp(baseBanana.meshes[0].vertices[i],
                                    danceBanana.meshes[0].vertices[i], phase);
        }
        UpdateMeshBuffer(morphBanana.meshes[0], 0, morphVertices.data(),
                         static_cast<int>(morphVertices.size() * sizeof(float)), 0);

        const float scale = maximized ? 0.65f : 0.50f;
        const float uiScale = maximized ? 1.12f : 1.0f;
        const int width = GetScreenWidth();
        const int height = GetScreenHeight();
        if (outlineLayer.texture.width != width || outlineLayer.texture.height != height) {
            UnloadRenderTexture(outlineLayer);
            UnloadRenderTexture(bananaLayer);
            outlineLayer = LoadRenderTexture(width, height);
            bananaLayer = LoadRenderTexture(width, height);
        }
        const Rectangle minButton{static_cast<float>(width) - 148.0f * uiScale, 19.0f, 52.0f * uiScale, 38.0f * uiScale};
        const Rectangle maxButton{static_cast<float>(width) - 91.0f * uiScale, 17.0f, 40.0f * uiScale, 40.0f * uiScale};
        const Rectangle closeButton{static_cast<float>(width) - 46.0f * uiScale, 16.0f, 35.0f * uiScale, 39.0f * uiScale};

        if (ButtonHit(minButton)) MinimizeWindow();
        if (ButtonHit(maxButton)) {
            maximized = !maximized;
            SetWindowSize(maximized ? 920 : kWindowWidth, maximized ? 720 : kWindowHeight);
        }
        if (ButtonHit(closeButton)) break;

        const Vector3 position{0.0f, -0.33f + hop, 0.0f};
        const Vector3 axis{0.0f, 1.0f, 0.0f};
        BeginTextureMode(outlineLayer);
        ClearBackground(BLANK);
        BeginMode3D(camera);
        DrawModelEx(morphBanana, position, axis, turn, {scale, scale, scale}, kInk);
        EndMode3D();
        EndTextureMode();

        BeginTextureMode(bananaLayer);
        ClearBackground(BLANK);
        BeginMode3D(camera);
        DrawModelEx(morphBanana, position, axis, turn, {scale, scale, scale}, WHITE);
        EndMode3D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLANK);
        DrawTexturePro(cardboard, {0, 0, static_cast<float>(cardboard.width), static_cast<float>(cardboard.height)},
                       {0, 0, static_cast<float>(width), static_cast<float>(height)}, {0, 0}, 0.0f, WHITE);

        const Rectangle source{0.0f, 0.0f, static_cast<float>(width), -static_cast<float>(height)};
        for (int y = -2; y <= 2; y += 2) {
            for (int x = -2; x <= 2; x += 2) {
                if (x != 0 || y != 0) {
                    DrawTexturePro(outlineLayer.texture, source,
                                   {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)},
                                   {0, 0}, 0.0f, WHITE);
                }
            }
        }
        DrawTexturePro(bananaLayer.texture, source, {0, 0, static_cast<float>(width), static_cast<float>(height)}, {0, 0}, 0.0f, WHITE);
        const Color titleColor = ColorFromHSV(fmodf(danceTime * 220.0f, 360.0f), 0.88f, 1.0f);
        DrawText("BANANA DANCING.EXE", 33, 31, 19, BLACK);
        DrawText("BANANA DANCING.EXE", 31, 29, 19, titleColor);
        DrawImageButton(minimize, minButton);
        DrawImageButton(maximize, maxButton);
        DrawImageButton(close, closeButton);
        EndDrawing();
    }

    UnloadModel(morphBanana);
    UnloadModel(danceBanana);
    UnloadModel(baseBanana);
    UnloadTexture(close);
    UnloadTexture(maximize);
    UnloadTexture(minimize);
    UnloadTexture(cardboard);
    UnloadRenderTexture(bananaLayer);
    UnloadRenderTexture(outlineLayer);
    CloseWindow();
    return 0;
}
