#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int kWidth = 300;
constexpr int kHeight = 240;

const std::vector<std::string> kPhonemeNames = {
    "AA", "AE", "AH", "AO", "AX", "ER", "EH", "IH", "IY", "UH",
    "UW", "AY", "AW", "EY", "OW", "OY", "B", "D", "DH", "F", "G",
    "HH", "JH", "K", "L", "M", "N", "NG", "P", "R", "S", "SH",
    "TH", "V", "W", "Y", "Z", "ZH", "CH", "T"};

const std::unordered_map<std::string, std::vector<std::string>> kDictionary = {
    {"a", {"AH"}}, {"am", {"AE", "M"}}, {"and", {"AE", "N", "D"}},
    {"are", {"AA", "R"}}, {"beautiful", {"B", "Y", "UW", "T", "IH", "F", "AH", "L"}},
    {"can", {"K", "AE", "N"}}, {"code", {"K", "OW", "D"}},
    {"cpp", {"S", "IY", "P", "L", "AH", "S", "P", "L", "AH", "S"}},
    {"dream", {"D", "R", "IY", "M"}}, {"english", {"IH", "NG", "G", "L", "IH", "SH"}},
    {"future", {"F", "Y", "UW", "CH", "ER"}}, {"hello", {"HH", "EH", "L", "OW"}},
    {"i", {"AY"}}, {"is", {"IH", "Z"}}, {"love", {"L", "AH", "V"}},
    {"music", {"M", "Y", "UW", "Z", "IH", "K"}}, {"my", {"M", "AY"}},
    {"name", {"N", "EY", "M"}}, {"play", {"P", "L", "EY"}},
    {"sing", {"S", "IH", "NG"}}, {"singer", {"S", "IH", "NG", "ER"}},
    {"the", {"DH", "AH"}}, {"this", {"DH", "IH", "S"}},
    {"to", {"T", "UW"}}, {"virtual", {"V", "ER", "CH", "UW", "AH", "L"}},
    {"voice", {"V", "OY", "S"}}, {"world", {"W", "ER", "L", "D"}},
    {"you", {"Y", "UW"}}, {"your", {"Y", "AO", "R"}}
};

bool IsKnownPhoneme(const std::string& token) {
    return std::find(kPhonemeNames.begin(), kPhonemeNames.end(), token) != kPhonemeNames.end();
}

std::vector<std::string> ApproximateWord(const std::string& word) {
    std::vector<std::string> out;
    for (size_t i = 0; i < word.size();) {
        const std::string pair = word.substr(i, std::min<size_t>(2, word.size() - i));
        if (pair == "ch") { out.push_back("CH"); i += 2; continue; }
        if (pair == "sh") { out.push_back("SH"); i += 2; continue; }
        if (pair == "th") { out.push_back("TH"); i += 2; continue; }
        if (pair == "ng") { out.push_back("NG"); i += 2; continue; }
        if (pair == "ph") { out.push_back("F"); i += 2; continue; }
        if (pair == "oo") { out.push_back("UW"); i += 2; continue; }
        if (pair == "ee") { out.push_back("IY"); i += 2; continue; }
        if (pair == "ai" || pair == "ay") { out.push_back("EY"); i += 2; continue; }
        if (pair == "ow" || pair == "oa") { out.push_back("OW"); i += 2; continue; }
        const char c = word[i++];
        const std::unordered_map<char, std::string> map = {
            {'a', "AE"}, {'b', "B"}, {'c', "K"}, {'d', "D"}, {'e', "EH"},
            {'f', "F"}, {'g', "G"}, {'h', "HH"}, {'i', "IH"}, {'j', "JH"},
            {'k', "K"}, {'l', "L"}, {'m', "M"}, {'n', "N"}, {'o', "AA"},
            {'p', "P"}, {'q', "K"}, {'r', "R"}, {'s', "S"}, {'t', "T"},
            {'u', "AH"}, {'v', "V"}, {'w', "W"}, {'x', "K"}, {'y', "Y"}, {'z', "Z"}};
        if (auto it = map.find(c); it != map.end()) out.push_back(it->second);
    }
    return out;
}

std::vector<std::string> TextToPhonemes(const std::string& text) {
    std::vector<std::string> result;
    std::string word;
    auto flush = [&] {
        if (word.empty()) return;
        std::string uppercase;
        uppercase.reserve(word.size());
        for (unsigned char c : word) uppercase += static_cast<char>(std::toupper(c));
        // A recognised label such as AA, AH, SH, or T is a complete sound.
        // It must not be interpreted as an English word made of individual letters.
        if (IsKnownPhoneme(uppercase)) {
            result.push_back(uppercase);
            word.clear();
            return;
        }
        auto it = kDictionary.find(word);
        auto sounds = it != kDictionary.end() ? it->second : ApproximateWord(word);
        result.insert(result.end(), sounds.begin(), sounds.end());
        word.clear();
    };
    for (unsigned char c : text) {
        if (std::isalpha(c)) word += static_cast<char>(std::tolower(c));
        else flush();
    }
    flush();
    return result;
}

class Voice {
public:
    void Load() {
        for (const auto& name : kPhonemeNames) {
            const std::string path = "assets/phonemes/" + name + ".wav";
            Wave wave = LoadWave(path.c_str());
            Sound sound = LoadSoundFromWave(wave);
            SetSoundVolume(sound, GainForWave(wave));
            durations_.emplace(name, static_cast<double>(wave.frameCount) / wave.sampleRate);
            UnloadWave(wave);
            sounds_.emplace(name, sound);
        }
    }

    void Unload() {
        Stop();
        for (auto& [_, sound] : sounds_) UnloadSound(sound);
    }

    void Speak(const std::string& text) {
        Stop();
        const auto phonemes = TextToPhonemes(text);
        for (const auto& p : phonemes) if (sounds_.count(p)) queue_.push_back(p);
        StartNext();
    }

    void Update() {
        if (!playing_) return;
        const double now = GetTime();
        if (!queue_.empty() && now >= nextStartTime_) {
            StartNext();
        } else if (queue_.empty() && now >= finishTime_ &&
                   !IsSoundPlaying(sounds_.at(current_))) {
            playing_ = false;
            current_.clear();
        }
    }

    void Stop() {
        for (auto& [_, sound] : sounds_) StopSound(sound);
        queue_.clear();
        current_.clear();
        playing_ = false;
        nextStartTime_ = 0.0;
        finishTime_ = 0.0;
    }

    bool Playing() const { return playing_; }
    const std::string& Current() const { return current_; }
private:
    static float GainForWave(const Wave& wave) {
        if (wave.sampleSize != 16 || wave.frameCount == 0) return 1.0f;
        const auto* samples = static_cast<const int16_t*>(wave.data);
        const size_t count = static_cast<size_t>(wave.frameCount) * wave.channels;
        double energy = 0.0;
        for (size_t i = 0; i < count; ++i) {
            const double value = static_cast<double>(samples[i]) / 32768.0;
            energy += value * value;
        }
        const double rms = std::sqrt(energy / static_cast<double>(count));
        if (rms < 0.002) return 1.0f;
        // 0.18 is a comfortable target RMS for the isolated voice samples.
        return static_cast<float>(std::clamp(0.18 / rms, 0.45, 1.8));
    }

    void StartNext() {
        if (queue_.empty()) { playing_ = false; current_.clear(); return; }
        current_ = queue_.front();
        queue_.pop_front();
        PlaySound(sounds_.at(current_));
        const double now = GetTime();
        const double duration = durations_.at(current_);
        // Begin the following sample before this one finishes. This removes the
        // silent gap caused by individually recorded phoneme tails.
        nextStartTime_ = now + duration * 0.75;
        finishTime_ = now + duration;
        playing_ = true;
    }

    std::unordered_map<std::string, Sound> sounds_;
    std::unordered_map<std::string, double> durations_;
    std::deque<std::string> queue_;
    std::string current_;
    bool playing_ = false;
    double nextStartTime_ = 0.0;
    double finishTime_ = 0.0;
};

bool PlayButton(Rectangle rect, Color color) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, hover ? ColorBrightness(color, 0.10f) : color);
    const float cx = rect.x + rect.width * 0.52f;
    const float cy = rect.y + rect.height * 0.5f;
    DrawTriangle({cx - 6, cy - 8}, {cx - 6, cy + 8}, {cx + 8, cy}, {255, 246, 211, 255});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void UseApplicationResources() {
    namespace fs = std::filesystem;
    const fs::path executableDirectory = GetApplicationDirectory();
    const fs::path bundledResources = executableDirectory / ".." / "Resources";

    // A macOS .app keeps assets in Contents/Resources. Portable Windows and
    // Linux builds keep the assets beside the executable in an assets folder.
    const fs::path resourceDirectory = fs::is_directory(bundledResources / "assets")
        ? bundledResources
        : executableDirectory;
    ChangeDirectory(resourceDirectory.string().c_str());
}

} // namespace

int main() {
    UseApplicationResources();
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(kWidth, kHeight, "appleguo voice");
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    SetTargetFPS(60);

    Model model = LoadModel("assets/models/appleguo.glb");
    RenderTexture2D portrait = LoadRenderTexture(82, 92);
    Texture2D appleTile = LoadTexture("assets/images/apple_new.png");
    // Sound playback state can remain "playing" after an OGG ends on some
    // backends. Drive the BGM loop from the decoded file's real duration instead.
    Wave backgroundWave = LoadWave("assets/music/background.ogg");
    const double backgroundDuration = static_cast<double>(backgroundWave.frameCount) /
                                    static_cast<double>(backgroundWave.sampleRate);
    Sound backgroundMusic = LoadSoundFromWave(backgroundWave);
    UnloadWave(backgroundWave);
    SetSoundVolume(backgroundMusic, 0.30f);
    PlaySound(backgroundMusic);
    double backgroundCycleStartedAt = GetTime();
    int backgroundCycle = 1;
    double backgroundRestartedAt = -1.0;
    Voice voice;
    voice.Load();
    std::string input = "HH EH L OW  W ER L D";
    std::string lastSentence = input;
    bool inputActive = true;
    while (!WindowShouldClose()) {
        const double nowTime = GetTime();
        // Restart a few milliseconds early so the next cycle is audible without
        // relying on IsSoundPlaying(), which can stay true after an OGG ends.
        if (nowTime - backgroundCycleStartedAt >= backgroundDuration - 0.01) {
            StopSound(backgroundMusic);
            PlaySound(backgroundMusic);
            backgroundCycleStartedAt = nowTime;
            backgroundRestartedAt = nowTime;
            ++backgroundCycle;
            TraceLog(LOG_INFO, "BGM loop restart: cycle %d at %.2f seconds", backgroundCycle, nowTime);
        }
        const double backgroundElapsed = nowTime - backgroundCycleStartedAt;
        voice.Update();
        Rectangle inputRect{10, 193, 220, 36};
        Rectangle speakRect{240, 193, 50, 36};

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) inputActive = CheckCollisionPointRec(GetMousePosition(), inputRect);
        if (inputActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 126 && input.size() < 80) input.push_back(static_cast<char>(key));
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && !input.empty()) input.pop_back();
        }
        bool requestSpeak = IsKeyPressed(KEY_ENTER);
        if (IsKeyPressed(KEY_ESCAPE)) voice.Stop();

        const float bob = std::sin(GetTime() * 1.8) * 0.045f;
        float scaleXz = 0.78f;
        float scaleY = 0.78f;
        if (voice.Playing()) {
            const float squash = 0.5f + 0.5f * std::sin(GetTime() * 17.0);
            scaleY *= 0.68f + squash * 0.18f;
            scaleXz *= 1.18f - squash * 0.10f;
        }

        Camera3D portraitCamera{};
        // The supplied Blender reference shows the character's front on the X axis.
        portraitCamera.position = {5.5f, 0.8f, 0};
        portraitCamera.target = {0, 0.3f, 0};
        portraitCamera.up = {0, 1, 0};
        portraitCamera.fovy = 38;
        portraitCamera.projection = CAMERA_PERSPECTIVE;
        BeginTextureMode(portrait);
        ClearBackground({0, 0, 0, 0});
        BeginMode3D(portraitCamera);
        // Shift the model upward inside its own portrait, without moving the UI card.
        DrawModelEx(model, {0, 0.22f + bob, 0}, {0, 1, 0},
                    std::sin(GetTime() * 1.4f) * 8.0f, {scaleXz, scaleY, scaleXz}, WHITE);
        EndMode3D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground({248, 204, 92, 255});
        const float flow = std::fmod(GetTime() * 9.0f, 48.0f);
        for (int y = -10; y < kHeight + 28; y += 38) {
            for (int x = -48; x < kWidth + 48; x += 48) {
                const float px = static_cast<float>(x) + flow + ((y / 38) % 2) * 24.0f;
                DrawTexturePro(appleTile, {0, 0, static_cast<float>(appleTile.width), static_cast<float>(appleTile.height)},
                               {px, static_cast<float>(y), 21, 30}, {0, 0}, 0, {255, 242, 191, 54});
                DrawLine(px + 25, static_cast<float>(y) + 13, px + 41, static_cast<float>(y) + 13,
                         {181, 120, 28, 30});
            }
        }
        DrawRectangle(10, 12, 84, 94, {250, 224, 143, 220});
        DrawTexturePro(portrait.texture, {0, 0, 82, -92}, {11, 13, 82, 92}, {0, 0}, 0, WHITE);
        DrawText("APPLEGUO", 12, 112, 10, {118, 78, 31, 255});

        DrawText("NOW SPEAKING", 112, 25, 10, {130, 83, 27, 255});
        const std::string now = voice.Playing() ? voice.Current() : "--";
        DrawText(now.c_str(), 110, 42, 42, voice.Playing() ? Color{188, 77, 31, 255} : Color{157, 109, 38, 255});
        DrawText(voice.Playing() ? "VOICE ACTIVE" : "WAITING", 113, 92, 12,
                 voice.Playing() ? Color{112, 94, 40, 255} : Color{148, 111, 46, 255});
        DrawText("BGM LOOP", 232, 109, 8, {138, 94, 31, 255});
        DrawLine(111, 118, 288, 118, {185, 128, 37, 150});
        const char* restartMark = nowTime - backgroundRestartedAt < 0.8 ? " R" : "";
        DrawText(TextFormat("%02d %04.1f/%04.1f%s", backgroundCycle,
                            backgroundElapsed, backgroundDuration, restartMark),
                 232, 120, 8, {138, 94, 31, 255});
        DrawText("PHONEME INPUT", 11, 176, 10, {126, 83, 27, 255});

        DrawRectangleRec(inputRect, inputActive ? Color{255, 238, 183, 255} : Color{252, 229, 157, 255});
        DrawRectangleLinesEx(inputRect, 1, {179, 119, 30, 255});
        std::string shown = input;
        while (MeasureText(shown.c_str(), 14) > inputRect.width - 18 && shown.size() > 1) shown.erase(shown.begin());
        DrawText(shown.c_str(), static_cast<int>(inputRect.x + 8), static_cast<int>(inputRect.y + 11), 14, {80, 58, 28, 255});
        if (inputActive && static_cast<int>(GetTime() * 2) % 2 == 0) {
            const int x = static_cast<int>(inputRect.x + 9 + MeasureText(shown.c_str(), 14));
            DrawRectangle(x, static_cast<int>(inputRect.y + 8), 1, 20, {166, 89, 24, 255});
        }
        if (PlayButton(speakRect, {192, 88, 31, 255})) requestSpeak = true;
        EndDrawing();

        if (requestSpeak && !input.empty()) {
            lastSentence = input;
            voice.Speak(lastSentence);
        }
    }

    voice.Unload();
    StopSound(backgroundMusic);
    UnloadSound(backgroundMusic);
    UnloadTexture(appleTile);
    UnloadRenderTexture(portrait);
    UnloadModel(model);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
