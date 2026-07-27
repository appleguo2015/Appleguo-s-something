# Virtual Singer

一个离线 C++ 虚拟歌姬原型。输入英文句子后，程序会把单词转换为 ARPAbet
音素，逐段播放 `assets/phonemes/` 中的采样，并在说话时挤压整个角色模型。

当前音素库最初来自 Scratch 工程，已提取为运行时 WAV，包括新补录的 `T.wav`。
原始 Scratch 与 Blender 制作文件已归档到 `BIBIBIBI/`，不参与构建或发布。
`assets/music/background.ogg` 会作为 30% 音量的循环背景音乐播放。它是 22.05 kHz 的低码率 OGG，适合 8-bit 风格并保持较小体积。

## 构建与运行

这是一个原生桌面程序，当前支持 macOS、Windows 和 Linux。三端共用同一份
C++ 源码、GLB 模型、音素采样与 BGM，不需要 CMake。

共同依赖：`make`、支持 C++17 的编译器、`pkg-config` 和 raylib 5.5。

| 平台 | 安装依赖 | 构建与运行 |
| --- | --- | --- |
| macOS | `brew install raylib pkg-config` | `make run` |
| Windows | 安装 [MSYS2](https://www.msys2.org/)，在 **MINGW64** 终端运行 `pacman -S --needed mingw-w64-x86_64-raylib mingw-w64-x86_64-pkgconf make` | `make run` |
| Ubuntu/Debian | `sudo apt install build-essential pkg-config libraylib-dev` | `make run` |

在 Windows 上必须从 MSYS2 的 MINGW64 终端运行 `make`；构建结果会自动命名为
`out/virtual_singer.exe`。macOS 和 Linux 则生成 `out/virtual_singer`。

## 发给朋友

macOS 上执行 `make app` 会创建 `dist/AppleGuo Voice.app` 和
`dist/appleguo-voice-macos.zip`。将 ZIP 发给朋友即可；解压后双击 `.app`。
应用包带有 raylib、模型和音频资源，不依赖你的 Homebrew 安装。

Windows 和 Linux 的可分发压缩包由 GitHub Actions 构建；见
`.github/workflows/release.yml`。Windows 用户解压 ZIP 后双击
`virtual_singer.exe`；ZIP 内已包含 raylib 和 MinGW 所需的 DLL。Linux 用户解压后
运行 `./virtual_singer`；CI 会静态链接 raylib，接收者无需另装 raylib。Windows 的
`.exe` 使用 `apple.png` 生成的程序图标；Linux 包含同一图标与
`appleguo-voice.desktop` 启动器。
首次将未签名的 macOS 应用发给他人时，Gatekeeper 可能会拦截。用户可在 Finder
中按住 Control 点击应用并选择“打开”。面向公开发布时，需要 Apple Developer ID
签名和公证。

```bash
make
make run
```

操作：

- 点击输入框并输入英文，按 `Enter` 或点击 `SPEAK`。
- 也可直接输入空格分隔的音素，例如 `HH EH L OW` 或 `AA AH T`；每个标签会作为一个完整音节播放，不会拆成字母。
- 相邻音素会在前一个音结束前开始，避免音与音之间出现人工静音。
- `Space` 重播当前句子，`Esc` 停止播放。
- 鼠标左键拖动旋转镜头，滚轮缩放。

## 当前实现边界

语音由孤立音素直接拼接，属于有意保留机械感的离线原型，并不是自然语言
TTS。内置词典覆盖演示句和常见词；其他单词使用规则式近似发音。要提升质量，
可以接入 Piper/ONNX TTS，并让 TTS 同时输出音素时间戳。

角色不使用骨骼或表情形态键。播放语音时，程序直接压缩 GLB 的纵向比例，同时
略微增大横向比例，产生软体角色说话时被压扁的效果。
