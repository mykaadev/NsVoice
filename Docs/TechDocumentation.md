# NsVoice Technical Documentation

## Architecture

### Modules

| Module | Type | Loading Phase | Content |
|---|---|---|---|
| `NsVoice` | Runtime | Default | The subsystems, the components, the log category |

The `.uplugin` sets `CanContainContent` to false. The plugin has no `Content/` and no `Config/` folder.

### Folders

- `Source/NsVoice/Public` – `NsMicSubsystem.h`, `NsVoiceSubsystem.h`, `NsVoiceChatComponent.h`, `NsVoiceActivityComponent.h`, `NsVoiceModule.h`.
- `Source/NsVoice/Private` – The `.cpp` files of the public headers.

### Main Classes

| Class | Base | Responsibility |
|---|---|---|
| `UNsMicSubsystem` | `UGameInstanceSubsystem` | Owns the one `Audio::FAudioCapture` stream, the picked device, the gain, the level, and one sample queue for each listener |
| `UNsVoiceSubsystem` | `UGameInstanceSubsystem` | Holds the volume of every voice and a map of player volumes by player name |
| `UNsVoiceChatComponent` | `UAudioComponent` | On the owner: reads, resamples, encodes, and sends the microphone. On the other machines: decodes and plays the voice. Computes the talk level |
| `UNsVoiceActivityComponent` | `UActorComponent` | Turns the microphone level into a replicated speech state with an event |
| `FNsVoiceModule` | `IModuleInterface` | Empty module class. `NsVoiceModule.cpp` defines the log category `LogNsVoice` |
| `FNsMicDevice` | plain struct | The `Id` and `Name` of one microphone |

The default of each setting is in the [User Guide](UserGuide.md).

### Data Flow

1. On the owner, `TickOwner` calls `SetTransmitting(true)` one time when the pawn is locally controlled and `bTransmitWhenLocallyControlled` is true. Game code can also call `SetTransmitting`.
2. `SetTransmitting(true)` creates the encoder with `FVoiceModule::CreateVoiceEncoder` (16 kHz, mono, `VoiceEncode_Voice`). Then it calls `UNsMicSubsystem::OpenListener(1.f)`.
3. `OpenListener` opens the capture stream if it is closed. The stream uses the picked device and a buffer of 1024 frames.
4. On the audio thread, `HandleAudio` mixes each block to mono, stores the block peak in an atomic, and appends the block to every listener queue under `ListenerMutex`. A full queue drops the new samples.
5. On the game thread, `TickComponent` calls `SendMicrophone`. It drains the queue, applies the gain, clamps to -1..1, and resamples with linear interpolation to 16 kHz `int16` PCM.
6. `SendMicrophone` encodes packets of `FramesPerPacket` frames of 20 ms (320 samples). The rest waits for the next tick. Each packet goes to the host with `ServerSendVoice`.
7. The host drops an empty packet or a packet larger than `MaxPacketBytes`. It calls `MulticastVoice` with the other packets.
8. `MulticastVoice` calls `PlayPacket` only when the net mode is not `NM_DedicatedServer` and the pawn is not locally controlled.
9. `PlayPacket` creates the decoder and a `USoundWaveProcedural` (16 kHz, mono, `SOUNDGROUP_Voice`) on the first packet. It decodes the packet, records its peak and time, and queues the PCM. When more than `MaxQueuedSeconds` of audio waits, it calls `ResetAudio` first. It calls `Play` if the component does not play.
10. Each tick, the component interpolates `TalkLevel` to the raw level with `FInterpTo`. It sets the volume multiplier from `GetPlaybackVolume` while it plays.
11. While it transmits, `TickOwner` gives `UNsMicSubsystem::GetLevel` to the `UNsVoiceActivityComponent` of the same pawn.
12. Each tick of the activity component on the locally controlled pawn compares the time since the last loud level with `SpeechHoldSeconds`. On a change, it sets `bSpeaking`, fires the events, and calls `ServerSetSpeaking`.

Threading: only `HandleAudio` runs on the audio thread. It reaches the subsystem through a `TWeakObjectPtr`. All other code runs on the game thread.

## Lifetime

- The engine creates both subsystems with the game instance. `UNsMicSubsystem::Deinitialize` closes the stream.
- The capture stream opens with the first listener and closes with the last listener. `CloseStream` sets the sample rate and the peak to 0.
- `SetDeviceId` with a new ID calls `ReopenDevice`. If the stream is open, it closes and opens again on the new device. Every listener queue empties and gets a new size from the new sample rate.
- If the picked device is not in the device list, `OpenStream` logs a warning and uses the default device.
- `UNsVoiceChatComponent` sets `bAutoActivate` to false. It starts to play on the first received packet.
- The automatic start runs one time per component (`bAutoTransmitDone`). If the microphone does not open, the component does not try again.
- `EndPlay` calls `SetTransmitting(false)`. This closes the listener and clears the PCM and resample buffers.
- The encoder, the decoder, and the sound wave live as long as the component.

## Extension Points

| Hook | Kind | Signature | Use |
|---|---|---|---|
| `GetPlaybackVolume` | `BlueprintNativeEvent`, protected | `virtual float GetPlaybackVolume_Implementation() const` | Volume rules such as a radio. The default returns `GetVoiceVolume() * GetPlayerVolume(PlayerName)`, or 1 without the subsystem |
| `OnSpeakingChanged` | native multicast delegate | `FOnNsSpeakingChanged(bool bSpeaking)` | C++ reactions to speech |
| `OnSpeakingChangedEvent` | `BlueprintAssignable`, shown as **On Speaking Changed** | `FOnNsSpeakingChangedDynamic(bool bSpeaking)` | Blueprint reactions to speech |
| `OpenListener`, `DrainSamples`, `CloseListener` | public functions | `int32 OpenListener(float MaxSeconds)`, `bool DrainSamples(int32 Listener, TArray<float>& Out)`, `void CloseListener(int32 Listener)` | A meter or a recorder that reads the same microphone stream |
| `SetLocalMicLevel` | public function | `void SetLocalMicLevel(float Level)` | Feed the activity component from another source |

Both components are `BlueprintSpawnableComponent` in the class group `NsVoice`.

## Dependencies

| Dependency | Kind | Why |
|---|---|---|
| `Core`, `CoreUObject`, `Engine` | Public module | Base engine types, subsystems, and components |
| `AudioCaptureCore` | Public module | `Audio::FAudioCapture` in `NsMicSubsystem.h` |
| `Voice` | Public module | `FVoiceModule`, `IVoiceEncoder`, and `IVoiceDecoder` (Opus) |
| `AudioCapture` | Private module | Capture backend |
| `AudioCapture` | Plugin, not optional | Enabled by the `.uplugin` |

The plugin depends on no other Ns plugin.

## Performance

- `UNsVoiceChatComponent` and `UNsVoiceActivityComponent` tick every frame.
- The audio thread does one mono mix of each block and one copy into each listener queue, under one lock.
- The owner listener holds 1 second of samples. The owner drains it every tick.
- `SendMicrophone` allocates one array of `MaxPacketBytes` for each packet. One packet holds 40 ms of voice by default, so a speaker sends about 25 packets each second.
- `PlayPacket` allocates a PCM buffer of four times the packet size for each decode.
- `GetPlaybackVolume` runs each tick while the component plays.
- `GetDevices` creates a temporary `FAudioCapture` on each call. `GetDeviceName` calls `GetDevices`. Do not call them each frame.

## Networking

- Both components call `SetIsReplicatedByDefault(true)`. The owner must be a replicated pawn that a player controls.
- `ServerSendVoice` is `Server, Unreliable`. `MulticastVoice` is `NetMulticast, Unreliable`. A lost packet makes a short gap.
- The host relays every packet. It reaches every client where the pawn is relevant.
- `bSpeaking` replicates with `COND_SkipOwner` and `OnRep_Speaking`. `ServerSetSpeaking` is `Server, Reliable`.
- The owner and the host call `OnRep_Speaking` directly. `bShownSpeaking` makes sure that each machine fires the events one time per change.
