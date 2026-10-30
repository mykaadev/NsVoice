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
  <a href="#-documentation">📚 Documentation</a> •
  <a href="#-credits">❤️ Credits</a> •
  <a href="#-support">📞 Support</a> •
  <a href="#-license">📃 License</a>
</p>
<a href="https://buymeacoffee.com/mykaadev"><img src="https://www.svgrepo.com/show/476855/coffee-to-go.svg" alt="Coffee" width="50px"></a>
<p><b>Buy me a coffee!</b></p>
</div>
<!-- GH_ONLY_END -->

## 👀 Summary
NsVoice is proximity voice chat for multiplayer games. A component on the player pawn reads the microphone, encodes the voice with Opus, and sends it through the host. The other machines play the voice from that component, so your attenuation settings make it quieter with distance.

NsVoice has no input bindings, no UI, and no save of the options. The game binds push to talk, draws the meter, and saves the picked microphone and the volumes.

## 📦 Features
- **Proximity voice**: The voice plays from an audio component on the speaker, with its attenuation.
- **Open microphone or push to talk**: The microphone opens when the player controls the pawn, or when the game calls `Set Transmitting`.
- **Opus codec**: 16 kHz mono voice in small packets, with a limit on the queued delay.
- **Microphone picker**: List the microphones and change the device during play (C++).
- **Gain and level**: Set the microphone gain and read the level for a meter.
- **Volumes**: One volume for all voices, and one volume for each player.
- **Speaking state**: `On Speaking Changed` fires on every machine when a player starts or stops speaking.
- **Talk level**: A smooth 0 to 1 level on every machine, to move a mouth, a head, or a light.
- **Game rules**: Override `GetPlaybackVolume`, for example for a radio.

## ⚙️ Requirements
- Unreal Engine 5.7+
- The engine plugin `AudioCapture`. NsVoice enables it.
- `bEnabled=true` in the `[Voice]` section of `DefaultEngine.ini`. Without it, the Opus encoder and decoder do not start.

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsVoice` folder into the `Plugins` folder of your project.
3. Add `"NsVoice"` to the dependencies of the game module that uses the components.
4. Add these lines to `Config/DefaultEngine.ini`:
   ```ini
   [Voice]
   bEnabled=true
   ```
5. Generate the project files and enable the plugin.

## 🚀 Getting Started
In Blueprint:
1. Open the Blueprint of the player character.
2. Add an `Ns Voice Chat` component, and attach it to the head.
3. Set an attenuation on the component in the Details panel.
4. Add an `Ns Voice Activity` component, if the game must know who speaks.
5. Start two players in a listen server or a client session, and talk.

**Transmit When Locally Controlled** is on, so the microphone opens for the local player. For push to talk, turn it off and call `Set Transmitting`.

In C++:
```cpp
#include "NsVoiceChatComponent.h"
#include "NsVoiceActivityComponent.h"

AMyHero::AMyHero()
{
    VoiceChat = CreateDefaultSubobject<UNsVoiceChatComponent>(TEXT("VoiceChat"));
    VoiceChat->SetupAttachment(GetMesh(), TEXT("head"));
    VoiceChat->bTransmitWhenLocallyControlled = false; // push to talk
    VoiceActivity = CreateDefaultSubobject<UNsVoiceActivityComponent>(TEXT("VoiceActivity"));
}

void AMyHero::StartTalk() { VoiceChat->SetTransmitting(true); }
void AMyHero::StopTalk() { VoiceChat->SetTransmitting(false); }
```

## 🔧 API
### Main Classes
- `UNsVoiceChatComponent` – An audio component. It sends the voice of the owner and plays the voice on the other machines.
- `UNsVoiceActivityComponent` – Replicates when a player speaks.
- `UNsMicSubsystem` – Game instance subsystem. It owns the microphone stream, the device, the gain, and the level.
- `UNsVoiceSubsystem` – Game instance subsystem. It holds the voice volume and the volume of each player.

### Main Functions
- `SetTransmitting` / `IsTransmitting` – Open or close the microphone of the owner.
- `GetTalkLevel` – The smooth talk level of a player, on every machine.
- `UNsMicSubsystem::GetDevices` / `SetDeviceId` – List the microphones and pick one (C++).
- `UNsMicSubsystem::SetGain` / `GetLevel` – Set the gain, and read the level for a meter.
- `SetVoiceVolume` / `SetPlayerVolume` – Set the volume of all voices, or of one player.
- `GetPlaybackVolume` – Override it in a child component for a game rule.

## 🚧 Limits
- The owner of the components must be a pawn.
- One microphone stream for each game instance.
- No echo cancellation and no noise gate.
- The host relays every packet to every machine where the pawn is relevant.
- The plugin does not save the picked microphone, the gain, or the volumes.

## 📚 Documentation
- [User Guide](https://mykaadev.github.io/plugins/nsvoice/user-guide/) – Setup, common tasks, settings, and debug tools.
- [Technical Documentation](https://mykaadev.github.io/plugins/nsvoice/tech-documentation/) – Architecture, lifetime, extension points, and tests.

<!-- GH_ONLY_START -->
## ❤️ Credits
<a href="https://github.com/mykaadev/NsVoice/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsVoice"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->
