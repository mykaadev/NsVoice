# NsVoice User Guide

## Purpose

NsVoice is proximity voice chat. It opens the microphone, encodes the voice with Opus, sends it through the host, and plays it from a component on the speaker. The player can pick the microphone and the volumes.

NsVoice has no input, no UI, and no options file. The game binds push to talk, shows the meter, and saves the options. NsVoice depends on no other studio plugin.

## Setup

1. Enable `NsVoice` in the `.uproject` file. It enables the engine plugin `AudioCapture`.
2. Add `NsVoice` to the module dependencies of the game.
3. In `DefaultEngine.ini`, set `bHasVoiceEnabled=false` under `[OnlineSubsystem]`, and `bEnabled=true` under `[Voice]`. The engine voice chat stays off, and the Opus codec stays on.
4. Set `ConfiguredInternetSpeed` and `ConfiguredLanSpeed` under `[/Script/Engine.Player]` to 300000 or more. Low rates drop voice packets.

The plugin has no project settings.

## Quick Start

```cpp
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

Blueprint: add `Ns Voice Chat Component` to the head of the character. Set its attenuation in the Details panel. The voice plays from that point.

## Common Tasks

### Pick the microphone

Call `UNsMicSubsystem::GetDevices` for the list. Give the `Id` of the picked device to `SetDeviceId`. An empty ID picks the Windows default. An open stream opens again on the new device at once.

### Set the volumes

Call `UNsVoiceSubsystem::SetVoiceVolume` for every voice. Call `SetPlayerVolume` with a player name for one player. A player volume lasts for the session.

### Show a level meter

Call `UNsMicSubsystem::GetLevel` each frame. Open a listener with `OpenListener` while the meter shows, so the stream stays open. Close it with `CloseListener`.

### Show who speaks

Give the microphone level to `UNsVoiceActivityComponent::SetLocalMicLevel` on the owner each frame. Bind `OnSpeakingChanged` on every machine.

### Move the mouth to the voice

Read `UNsVoiceChatComponent::GetVoiceLevel` on the other machines. On the owner, read `UNsMicSubsystem::GetLevel` while the player transmits.

### Change the volume with a game rule

Override `GetPlaybackVolume` in a child component. For example, a radio returns full volume at any distance. Call the parent to keep the player volumes.

## Rules

- Make one voice chat component for each player. The owner sends, and every other machine plays.
- Close every listener that you open. The stream stays open while one listener is open.
- Keep push to talk, the meter, and the options in the game.

## Settings

On `UNsVoiceChatComponent`:

| Setting | Default | Effect |
|---|---|---|
| `FramesPerPacket` | 2 | Opus frames of 20 ms in one packet |
| `MaxPacketBytes` | 2000 | The host drops a larger packet |
| `MaxQueuedSeconds` | 0.4 | Older voice drops, so the delay does not grow |
| `LevelHoldSeconds` | 0.15 | Time after the last packet until the voice level is zero |

On `UNsVoiceActivityComponent`:

| Setting | Default | Effect |
|---|---|---|
| `SpeechLevel` | 0.12 | Microphone level that counts as speech |
| `SpeechHoldSeconds` | 0.4 | Short pauses do not stop the speech |

On `UNsMicSubsystem`: the gain goes from 0 to 4, and the default is 1.5.

## Debug

- Log category `LogNsVoice`: a picked microphone that is not connected, and a microphone that does not open.
- Run the test `NsVoice.Codec` in Session Frontend.

## Limits

- One microphone stream for each game instance.
- No echo cancellation and no noise gate. The gain and `SpeechLevel` are the only controls.
- The host relays every packet. A dedicated server pays the bandwidth of every voice.
