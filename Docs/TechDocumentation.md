# NsVoice Technical Documentation

## Architecture

| Type | Kind | Job |
|---|---|---|
| `UNsMicSubsystem` | game instance subsystem | The one microphone stream, the device, the gain, and the listener queues |
| `UNsVoiceSubsystem` | game instance subsystem | The voice volume and the volume of each player |
| `UNsVoiceChatComponent` | audio component | Encodes, sends, decodes, and plays the voice |
| `UNsVoiceActivityComponent` | actor component | Replicates the speech state |

Flow:

1. The owner calls `SetTransmitting(true)`. The component opens a listener on `UNsMicSubsystem`.
2. The audio thread mixes each block to mono and adds it to every listener queue.
3. Each frame, the owner drains its queue, applies the gain, resamples it to 16 kHz, and encodes full Opus frames.
4. `ServerSendVoice` sends a packet to the host. The host checks the size and calls `MulticastVoice`.
5. The other machines decode the packet into a procedural sound wave. The component plays it with its attenuation.
6. Each frame, the component sets its volume from `GetPlaybackVolume`.

## Lifetime

- The stream opens with the first listener and closes with the last listener.
- `SetDeviceId` opens the stream again on the new device. The listener queues start again at the new sample rate.
- `EndPlay` stops the transmission. `Deinitialize` closes the stream.

## Extension Points

| Hook | Kind | Use |
|---|---|---|
| `GetPlaybackVolume` | Blueprint native event | Radio, cockpit muffle, or team rules |
| `OnSpeakingChanged` | event | Lights, icons, and NPC reactions |
| `OpenListener`, `DrainSamples` | functions | A recorder or a meter that reads the same stream |

## Dependencies

Engine modules `Core`, `CoreUObject`, `Engine`, `AudioCaptureCore`, and `Voice`, and the engine plugin `AudioCapture`. No studio plugin.

## Performance

- The audio thread does one mono mix and one copy for each listener. It holds a lock only for the copy.
- The owner encodes at 16 kHz mono. One packet carries 40 ms of voice by default.
- The queue of a listener has a fixed size. New samples drop when the game thread does not read them.

## Networking

- Voice packets are unreliable. A lost packet is a short gap, not a delay.
- The host relays the voice. It drops a packet larger than `MaxPacketBytes`.
- The speech state is reliable and skips the owner.
- Playback runs only on machines that do not own the component.

## Testing

- `NsVoice.Codec`: a tone goes through the encoder and the decoder, and the output keeps its loudness and its length.

## Change Rules

- Keep the packet format and the sample rate the same on every machine of one build.
- Add a hook with a default that keeps the current behavior.
- Keep input, UI, and option files out of the plugin.
