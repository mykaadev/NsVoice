# NsVoice User Guide

## Purpose

NsVoice gives proximity voice chat to a multiplayer game. A component on the player pawn reads the microphone, encodes the voice with Opus, and sends it through the host. The other machines play the voice from that component, with its attenuation. The player can pick the microphone, the gain, and the volumes.

NsVoice does not bind input, draw UI, or save options. The game binds push to talk, draws the meter, and saves the picked microphone, the gain, and the volumes. NsVoice does not use the voice chat of the online subsystem.

## Setup

1. Copy the `NsVoice` folder into the `Plugins` folder of your project.
2. Add `"NsVoice"` to the dependencies of the game module that uses the components.
3. Open `Config/DefaultEngine.ini`.
4. Add `bEnabled=true` to the `[Voice]` section.
5. Generate the project files.
6. Enable the plugin in **Edit > Plugins**. The engine plugin `AudioCapture` turns on with it.

The `[Voice]` line is required. Without it, the engine does not create the Opus encoder and decoder.

These lines are a common setup for this kind of voice chat. They are not required by the code:

```ini
[OnlineSubsystem]
bHasVoiceEnabled=false

[/Script/Engine.Player]
ConfiguredInternetSpeed=300000
ConfiguredLanSpeed=300000
```

The first line keeps the engine voice chat off. The other two lines give the connection more bandwidth for the voice packets.

The plugin has no project settings page.

## Quick Start

In Blueprint:

1. Open the Blueprint of the player character.
2. In the **Components** panel, click **Add** and search for `Ns Voice Chat`.
3. Attach the new component to the head socket of the mesh.
4. In the Details panel of the component, set an **Attenuation Settings** asset, or turn on **Override Attenuation**.
5. Start Play In Editor with two players and **Net Mode** set to **Play As Listen Server**.
6. Talk into the microphone. The other player hears the voice from the head of your character.

In C++:

```cpp
#include "NsVoiceChatComponent.h"

AMyHero::AMyHero()
{
    VoiceChat = CreateDefaultSubobject<UNsVoiceChatComponent>(TEXT("VoiceChat"));
    VoiceChat->SetupAttachment(GetMesh(), TEXT("head"));
}
```

**Transmit When Locally Controlled** is on by default. The microphone opens one time, when this machine controls the pawn.

## Common Tasks

### Use push to talk

1. Select the `Ns Voice Chat` component.
2. Turn off **Transmit When Locally Controlled**.
3. On the key press, call `Set Transmitting` with **New Transmitting** on.
4. On the key release, call `Set Transmitting` with **New Transmitting** off.

`Is Transmitting` returns true while the microphone is open.

### Pick the microphone (C++)

1. Call `UNsMicSubsystem::GetDevices()` to get the list. Each `FNsMicDevice` has an `Id` and a `Name`.
2. Show the `Name` values to the player.
3. Call `UNsMicSubsystem::Get(this)->SetDeviceId(Device.Id)` with the picked device.
4. Save the ID in your options. On the next start, call `SetDeviceId` again.

An empty ID picks the default microphone. An open stream opens again on the new device at once. `GetDeviceName` returns the name of a saved ID.

### Set the microphone gain

1. Get the subsystem with the `Get Game Instance Subsystem` node and the class `NsMicSubsystem`.
2. Call `Set Gain` with a value from 0 to 4.

The gain applies to the meter, the talk level, and the voice that the other players hear.

### Show a microphone meter

1. Get `NsMicSubsystem`.
2. Read `Get Level` each frame. The value goes from 0 to 1, after the gain.

`Get Level` returns 0 while the microphone is closed. In C++, call `OpenListener` to keep the microphone open for a meter, and `CloseListener` when the meter closes.

### Set the volumes

1. Get the subsystem with the `Get Game Instance Subsystem` node and the class `NsVoiceSubsystem`.
2. Call `Set Voice Volume` with a value from 0 to 2. It applies to every voice.
3. Call `Set Player Volume` with the player name and a value from 0 to 2. It applies to one player.

The player name is the name in the player state of the speaker. A player volume lasts for the session.

### Show who speaks

1. Add an `Ns Voice Activity` component to the same pawn as the `Ns Voice Chat` component.
2. Bind the event `On Speaking Changed` of the activity component.
3. Read `Is Speaking` at any time.

The event fires on every machine. The voice chat component gives the microphone level to the activity component while the microphone is open.

### Move a mouth or a light with the voice

1. Read `Get Talk Level` on the `Ns Voice Chat` component each frame.
2. Drive the mouth, the head, or the light with the value from 0 to 1.

On the speaker, the value comes from the microphone. On the other machines, it comes from the voice that the machine hears. `Get Voice Level` gives the raw level of the heard voice.

### Change the volume with a game rule

1. Make a C++ child class of `UNsVoiceChatComponent`.
2. Override `GetPlaybackVolume_Implementation`.
3. Return the volume for your rule, for example full volume for a radio.
4. Call `Super::GetPlaybackVolume_Implementation()` to keep the voice volume and the player volume.

## Settings

`Ns Voice Chat` component (`UNsVoiceChatComponent`), category **Voice**:

| Setting | Default | Effect |
|---|---|---|
| **Transmit When Locally Controlled** | On | Opens the microphone when this machine controls the pawn. Turn it off for push to talk |
| **Frames Per Packet** | 2 | Opus frames of 20 ms in one packet, from 1 to 5. Edit it in the Blueprint defaults only |
| **Max Packet Bytes** | 2000 | The host drops a larger packet. Edit it in the Blueprint defaults only |
| **Max Queued Seconds** | 0.4 | When more voice waits to play, the old voice drops. Edit it in the Blueprint defaults only |
| **Level Hold Seconds** | 0.15 | Time after the last packet until `Get Voice Level` is 0. Edit it in the Blueprint defaults only |

`Ns Voice Chat` component, category **Voice > Talk Level**:

| Setting | Default | Effect |
|---|---|---|
| **Remote Level Gain** | 10 | Multiplier of the heard voice level in the talk level |
| **Talk Rise Speed** | 18 | How fast the talk level goes up |
| **Talk Fall Speed** | 6 | How fast the talk level goes down |

`Ns Voice Activity` component (`UNsVoiceActivityComponent`), category **Voice**:

| Setting | Default | Effect |
|---|---|---|
| **Speech Level** | 0.12 | Microphone level that counts as speech, from 0.01 to 1 |
| **Speech Hold Seconds** | 0.4 | Speech stays on for this time after the level drops, so short pauses do not stop it |

Subsystems, set at runtime:

| Setting | Default | Effect |
|---|---|---|
| Gain (`NsMicSubsystem`) | 1.5 | Microphone gain, from 0 to 4 |
| Voice Volume (`NsVoiceSubsystem`) | 1 | Volume of every voice, from 0 to 2 |
| Player Volume (`NsVoiceSubsystem`) | 1 | Volume of one player, from 0 to 2 |

## Debug

- Log category `LogNsVoice`. Set it to `Verbose` with `Log LogNsVoice Verbose` to see each received packet, with its size and peak.
- At `Log` level, `LogNsVoice` shows the sample rate when the microphone opens, and warnings when a microphone does not open.
- The automation test `NsVoice.Codec` makes sure that the Opus encoder and decoder work. Run it in **Tools > Session Frontend > Automation**.

## Troubleshooting

| Problem | Cause | Fix |
|---|---|---|
| Ensure: "The voice encoder did not start" or "The voice decoder did not start" | The `[Voice]` section of `DefaultEngine.ini` does not have `bEnabled=true` | Add `bEnabled=true` to the `[Voice]` section, then restart the editor |
| Log: "No microphone opened. Voice chat stays off." | No microphone is connected, or the device does not open | Connect a microphone, then call `Set Transmitting` again. The automatic start runs one time only |
| Log: "The picked microphone ... is not connected" | The saved device ID is not in the device list | The game uses the default microphone. Pick a device again |
| No one hears the voice | The owner of the component is not a pawn, or the pawn is not locally controlled | Put the component on the pawn that the player controls |
| The voice is equally loud at every distance | The component has no attenuation | Set an attenuation on the component |
| `Is Speaking` stays false | The pawn has no `Ns Voice Chat` component, or its microphone is closed | Add both components to the same pawn, and open the microphone |
| `Is Speaking` stays true or does not start | **Speech Level** does not match the microphone | Change **Speech Level**, or change the gain |

## Limits

- The owner of both components must be a pawn.
- One microphone stream for each game instance.
- No echo cancellation and no noise gate.
- The speaker does not hear its own voice. A dedicated server does not play voice.
- The host relays every packet to every machine where the pawn is relevant.
- Player volumes use the player name. Two players with the same name share one volume.
- The plugin does not save the picked microphone, the gain, or the volumes.
- `GetDevices`, `SetDeviceId`, `OpenListener`, and `CloseListener` are C++ only.
