<!-- GH_ONLY_START -->
<h1 align="center">
  <br>
  <a href="https://github.com/mykaadev/NsVoice"><img src="https://raw.githubusercontent.com/mykaadev/NsVoice/refs/heads/main/Resources/Banner.png" alt="NsVoice" width="400"></a>
</h1>

<h4 align="center">Proximity voice chat with Opus and a microphone picker for Unreal Engine.</h4>

<div align="center">
    <a href="https://github.com/mykaadev/NsVoice/commits/main"><img src="https://img.shields.io/github/last-commit/mykaadev/NsVoice?style=plastic&logo=github&logoColor=white" alt="GitHub Last Commit"></a>
    <a href="https://github.com/mykaadev/NsVoice/issues"><img src="https://img.shields.io/github/issues-raw/mykaadev/NsVoice?style=plastic&logo=github&logoColor=white" alt="GitHub Issues"></a>
    <a href="https://github.com/mykaadev/NsVoice/pulls"><img src="https://img.shields.io/github/issues-pr-raw/mykaadev/NsVoice?style=plastic&logo=github&logoColor=white" alt="GitHub Pull Requests"></a>
    <a href="https://github.com/mykaadev/NsVoice"><img src="https://img.shields.io/github/stars/mykaadev/NsVoice?style=plastic&logo=github" alt="GitHub Stars"></a>
    <a href="https://twitter.com/mykaadev/"><img src="https://img.shields.io/twitter/follow/mykaadev?style=plastic&logo=x" alt="Twitter Follow"></a>

<p style="display:none;">
  <a href="#-summary">👀 Summary</a> •
  <a href="#-features">📦 Features</a> •
  <a href="#-requirements">⚙️ Requirements</a> •
  <a href="#-installation">🛠️ Installation</a> •
  <a href="#-getting-started">🚀 Getting Started</a> •
  <a href="#-api">🔧 API</a> •
  <a href="#-limits">🚧 Limits</a> •
  <a href="#-credits">❤️ Credits</a> •
  <a href="#-support">📞 Support</a> •
  <a href="#-license">📃 License</a>
</p>
<a href="https://buymeacoffee.com/mykaadev"><img src="https://www.svgrepo.com/show/476855/coffee-to-go.svg" alt="Coffee" width="50px"></a>
<p><b>Buy me a coffee!</b></p>
</div>
<!-- GH_ONLY_END -->

## 👀 Summary
NsVoice is proximity voice chat for Unreal Engine. It opens the microphone, encodes the voice with Opus, sends it through the host, and plays it from a component on the speaker. The player can pick the microphone and the volumes.

NsVoice has no input, no UI, and no options file. The game binds push to talk, shows the meter, and saves the options.

## 📦 Features
- **Proximity voice**: The voice plays from a component on the speaker, with your attenuation settings.
- **Opus codec**: Small voice packets, with a limit on the queued delay.
- **Microphone picker**: List the devices and change the device during play.
- **Volumes**: One volume for all voices, and one volume for each player.
- **Level meter**: Read the microphone level each frame.
- **Speaking state**: `OnSpeakingChanged` fires on every machine.
- **Lip sync input**: Read the voice level of a speaker to move the mouth.
- **Game rules**: Override `GetPlaybackVolume`, for example for a radio.

## ⚙️ Requirements
Unreal Engine 5.7+

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsVoice` folder into the `Plugins` folder of your project.
3. Add `"NsVoice"` to the module dependencies in `Build.cs`.
4. Generate the project files and enable the plugin. It enables `AudioCapture`.
5. In `DefaultEngine.ini`, turn off the engine voice chat and keep the Opus codec on:

```ini
[OnlineSubsystem]
bHasVoiceEnabled=false

[Voice]
bEnabled=true

[/Script/Engine.Player]
ConfiguredInternetSpeed=300000
ConfiguredLanSpeed=300000
```

Low net rates drop voice packets.

## 🚀 Getting Started
```cpp
#include "NsVoiceChatComponent.h"
#include "NsVoiceActivityComponent.h"

AMyHero::AMyHero()
{
    VoiceChat = CreateDefaultSubobject<UNsVoiceChatComponent>(TEXT("VoiceChat"));
    VoiceChat->SetupAttachment(GetMesh(), TEXT("head"));
    VoiceActivity = CreateDefaultSubobject<UNsVoiceActivityComponent>(TEXT("VoiceActivity"));
}

void AMyHero::StartTalk()
{
    VoiceChat->SetTransmitting(true);
}

void AMyHero::StopTalk()
{
    VoiceChat->SetTransmitting(false);
}
```

In Blueprint, add `Ns Voice Chat Component` to the head of the character. Set its attenuation in the Details panel.

## 🔧 API
### Key Classes
- `UNsVoiceChatComponent` – Sends the voice of the owner, and plays the voice on every other machine.
- `UNsVoiceActivityComponent` – Tells when a player speaks.
- `UNsMicSubsystem` – Owns the microphone stream, the device, the gain, and the level.
- `UNsVoiceSubsystem` – Owns the voice volume and the volume of each player.

### Main Functions
- `SetTransmitting` – Start or stop push to talk.
- `GetDevices` / `SetDeviceId` – List the microphones and pick one. An empty ID picks the system default.
- `GetLevel` – Read the microphone level, for a meter.
- `OpenListener` / `CloseListener` – Keep the stream open while a meter shows.
- `SetVoiceVolume` / `SetPlayerVolume` – Set the volume of all voices, or of one player.
- `GetVoiceLevel` – Read the voice level of a speaker on the other machines.
- `GetPlaybackVolume` – Override in a child component for a game rule.

### Settings
| Setting | Default | Effect |
|---|---|---|
| `FramesPerPacket` | 2 | Opus frames of 20 ms in one packet |
| `MaxPacketBytes` | 2000 | The host drops a larger packet |
| `MaxQueuedSeconds` | 0.4 | Older voice drops, so the delay does not grow |
| `SpeechLevel` | 0.12 | Microphone level that counts as speech |
| `SpeechHoldSeconds` | 0.4 | Short pauses do not stop the speech |

## 🚧 Limits
- One microphone stream for each game instance.
- No echo cancellation and no noise gate.
- The host relays every packet. A dedicated server pays the bandwidth of every voice.

<!-- GH_ONLY_START -->
## 📚 Documentation
- [User Guide](Docs/UserGuide.md) – Setup, common tasks, settings, and debug tools.
- [Technical Documentation](Docs/TechDocumentation.md) – Architecture, lifetime, extension points, and tests.

## ❤️ Credits
<a href="https://github.com/mykaadev/NsVoice/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsVoice"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->
