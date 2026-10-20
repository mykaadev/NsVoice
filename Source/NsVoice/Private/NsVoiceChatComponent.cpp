// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVoiceChatComponent.h"

#include "NsMicSubsystem.h"
#include "NsVoiceSubsystem.h"
#include "NsVoiceModule.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/VoiceCodec.h"
#include "Sound/SoundWaveProcedural.h"
#include "VoiceModule.h"

UNsVoiceChatComponent::UNsVoiceChatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    bAutoActivate = false;
}

void UNsVoiceChatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (IsTransmitting())
    {
        SendMicrophone();
    }
    if (IsPlaying())
    {
        SetVolumeMultiplier(GetPlaybackVolume());
    }
}

void UNsVoiceChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetTransmitting(false);
    Super::EndPlay(EndPlayReason);
}

void UNsVoiceChatComponent::SetTransmitting(const bool bNewTransmitting)
{
    UNsMicSubsystem* Mic = UNsMicSubsystem::Get(this);
    if (bNewTransmitting == IsTransmitting() || Mic == nullptr)
    {
        return;
    }
    if (!bNewTransmitting)
    {
        Mic->CloseListener(MicListener);
        MicListener = INDEX_NONE;
        PendingPcm.Reset();
        ResampleTail.Reset();
        ResamplePosition = 0.0;
        return;
    }
    if (!Encoder.IsValid())
    {
        Encoder = FVoiceModule::Get().CreateVoiceEncoder(VoiceSampleRate, 1, EAudioEncodeHint::VoiceEncode_Voice);
    }
    if (!ensureMsgf(Encoder.IsValid(), TEXT("The voice encoder did not start. Set bEnabled=true in the [Voice] section of DefaultEngine.ini.")))
    {
        return;
    }
    // One second of room is plenty: the owner reads the queue every frame
    MicListener = Mic->OpenListener(1.f);
    UE_LOG(LogNsVoice, Log, TEXT("Voice chat sends the microphone at %d Hz."), Mic->GetSampleRate());
}

float UNsVoiceChatComponent::GetVoiceLevel() const
{
    return GetWorld()->GetTimeSeconds() - LastPacketTime <= LevelHoldSeconds ? LastPeak : 0.f;
}

void UNsVoiceChatComponent::SendMicrophone()
{
    UNsMicSubsystem* Mic = UNsMicSubsystem::Get(this);
    TArray<float> Samples;
    if (Mic == nullptr || !Mic->DrainSamples(MicListener, Samples) || Mic->GetSampleRate() <= 0)
    {
        return;
    }
    const float Gain = Mic->GetGain();
    for (float& Sample : Samples)
    {
        Sample = FMath::Clamp(Sample * Gain, -1.f, 1.f);
    }
    Resample(Samples, Mic->GetSampleRate());

    // Opus encodes whole frames only. The rest waits for the next frame
    const int32 PacketBytes = FramesPerPacket * FrameSamples * sizeof(int16);
    while (PendingPcm.Num() >= PacketBytes)
    {
        TArray<uint8> Packet;
        Packet.SetNumUninitialized(MaxPacketBytes);
        uint32 PacketSize = Packet.Num();
        const int32 Left = Encoder->Encode(PendingPcm.GetData(), PacketBytes, Packet.GetData(), PacketSize);
        PendingPcm.RemoveAt(0, PacketBytes - Left, EAllowShrinking::No);
        if (PacketSize == 0)
        {
            break;
        }
        Packet.SetNum(PacketSize);
        ServerSendVoice(Packet);
    }
}

void UNsVoiceChatComponent::Resample(const TArray<float>& Input, const int32 InputRate)
{
    ResampleTail.Append(Input);
    const double Step = static_cast<double>(InputRate) / VoiceSampleRate;
    while (ResamplePosition + 1.0 < ResampleTail.Num())
    {
        const int32 Index = FMath::FloorToInt(ResamplePosition);
        const float Alpha = static_cast<float>(ResamplePosition - Index);
        const float Value = FMath::Lerp(ResampleTail[Index], ResampleTail[Index + 1], Alpha);
        const int16 Pcm = static_cast<int16>(FMath::RoundToInt(Value * 32767.f));
        PendingPcm.Append(reinterpret_cast<const uint8*>(&Pcm), sizeof(int16));
        ResamplePosition += Step;
    }
    const int32 Used = FMath::Min(FMath::FloorToInt(ResamplePosition), ResampleTail.Num());
    ResampleTail.RemoveAt(0, Used, EAllowShrinking::No);
    ResamplePosition -= Used;
}

void UNsVoiceChatComponent::ServerSendVoice_Implementation(const TArray<uint8>& Packet)
{
    if (Packet.Num() > 0 && Packet.Num() <= MaxPacketBytes)
    {
        MulticastVoice(Packet);
    }
}

void UNsVoiceChatComponent::MulticastVoice_Implementation(const TArray<uint8>& Packet)
{
    // The speaker does not hear itself, and a dedicated server has no speakers
    const APawn* Pawn = GetOwner<APawn>();
    if (GetNetMode() != NM_DedicatedServer && IsValid(Pawn) && !Pawn->IsLocallyControlled())
    {
        PlayPacket(Packet);
    }
}

void UNsVoiceChatComponent::PlayPacket(const TArray<uint8>& Packet)
{
    if (!Decoder.IsValid())
    {
        Decoder = FVoiceModule::Get().CreateVoiceDecoder(VoiceSampleRate, 1);
        if (!ensureMsgf(Decoder.IsValid(), TEXT("The voice decoder did not start. Set bEnabled=true in the [Voice] section of DefaultEngine.ini.")))
        {
            return;
        }
    }
    TArray<uint8> Pcm;
    Pcm.SetNumUninitialized(FramesPerPacket * FrameSamples * sizeof(int16) * 4);
    uint32 PcmSize = Pcm.Num();
    Decoder->Decode(Packet.GetData(), Packet.Num(), Pcm.GetData(), PcmSize);
    if (PcmSize == 0)
    {
        return;
    }

    // The loudest sample of the packet drives the mouth and the head of the speaker
    const int16* Samples = reinterpret_cast<const int16*>(Pcm.GetData());
    int32 Peak = 0;
    for (uint32 Index = 0; Index < PcmSize / sizeof(int16); ++Index)
    {
        Peak = FMath::Max(Peak, FMath::Abs(static_cast<int32>(Samples[Index])));
    }
    LastPeak = Peak / 32767.f;
    LastPacketTime = GetWorld()->GetTimeSeconds();
    UE_LOG(LogNsVoice, Verbose, TEXT("Voice packet of %d bytes from %s, peak %.2f."), Packet.Num(), *GetNameSafe(GetOwner()), LastPeak);

    if (!IsValid(VoiceWave))
    {
        VoiceWave = NewObject<USoundWaveProcedural>(this);
        VoiceWave->SetSampleRate(VoiceSampleRate);
        VoiceWave->NumChannels = 1;
        VoiceWave->Duration = INDEFINITELY_LOOPING_DURATION;
        VoiceWave->SoundGroup = SOUNDGROUP_Voice;
        SetSound(VoiceWave);
    }

    // A late burst of packets drops the old voice, so the delay stays short
    const int32 MaxQueuedBytes = FMath::CeilToInt(MaxQueuedSeconds * VoiceSampleRate) * sizeof(int16);
    if (VoiceWave->GetAvailableAudioByteCount() > MaxQueuedBytes)
    {
        VoiceWave->ResetAudio();
    }
    VoiceWave->QueueAudio(Pcm.GetData(), PcmSize);
    if (!IsPlaying())
    {
        SetVolumeMultiplier(GetPlaybackVolume());
        Play();
    }
}

float UNsVoiceChatComponent::GetPlaybackVolume_Implementation() const
{
    const UNsVoiceSubsystem* const Voice = UNsVoiceSubsystem::Get(this);
    const APawn* const Pawn = GetOwner<APawn>();
    const APlayerState* const State = IsValid(Pawn) ? Pawn->GetPlayerState() : nullptr;
    if (Voice == nullptr)
    {
        return 1.f;
    }
    return Voice->GetVoiceVolume() * (IsValid(State) ? Voice->GetPlayerVolume(State->GetPlayerName()) : 1.f);
}
