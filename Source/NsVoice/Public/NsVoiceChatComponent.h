// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "NsVoiceChatComponent.generated.h"

/**
 * Voice chat of one player. It uses the microphone of UNsMicSubsystem
 * On the owner it reads the microphone, encodes it with Opus, and sends it to the host. The host sends it to every machine
 * On the other machines it decodes the voice and plays it from this component. The Blueprint places it and sets its attenuation
 */
UCLASS(ClassGroup = (NsVoice), meta = (BlueprintSpawnableComponent))
class NSVOICE_API UNsVoiceChatComponent : public UAudioComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVoiceChatComponent();

    //~ Begin UActorComponent Interface
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

    /** Owner: starts or stops sending the microphone. Use it for push to talk, with Transmit When Locally Controlled off */
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void SetTransmitting(bool bNewTransmitting);

    /** Owner: returns true while the microphone is open and sends */
    UFUNCTION(BlueprintPure, Category = "Voice")
    bool IsTransmitting() const { return MicListener != INDEX_NONE; }

    /** Returns the loudness of the voice that this machine hears now, from 0 to 1 */
    UFUNCTION(BlueprintPure, Category = "Voice")
    float GetVoiceLevel() const;

    /**
     * Returns how loud this player talks now, from 0 to 1, on every machine. The value is smooth, so it can drive a mouth, a head, or a light
     * The owner reads its microphone. Every other machine reads the voice that it hears
     */
    UFUNCTION(BlueprintPure, Category = "Voice")
    float GetTalkLevel() const { return TalkLevel; }

protected:
    /** Returns the playback volume. The default is the voice volume of UNsVoiceSubsystem times the volume of this player. Override it for game rules, such as a radio */
    UFUNCTION(BlueprintNativeEvent, Category = "Voice")
    float GetPlaybackVolume() const;

    virtual float GetPlaybackVolume_Implementation() const;

private:
    /** Returns the talk level of this frame before the smoothing */
    float GetRawTalkLevel() const;

    /** Owner: opens the microphone one time when this machine controls the pawn, and gives its level to the voice activity component */
    void TickOwner();

    /** Owner: sends one packet of encoded voice to the host */
    UFUNCTION(Server, Unreliable)
    void ServerSendVoice(const TArray<uint8>& Packet);

    /** Host: sends one packet of encoded voice to every machine */
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastVoice(const TArray<uint8>& Packet);

    /** Owner: reads the microphone, encodes full frames, and sends them */
    void SendMicrophone();

    /** Turns microphone samples into voice-rate samples. A position between two samples carries to the next block */
    void Resample(const TArray<float>& Input, int32 InputRate);

    /** Plays one decoded packet on this machine */
    void PlayPacket(const TArray<uint8>& Packet);


// Variables
public:
    /** Sample rate of the voice on the network */
    static constexpr int32 VoiceSampleRate = 16000;

    /** Voice samples in one Opus frame: 20 milliseconds */
    static constexpr int32 FrameSamples = VoiceSampleRate / 50;

    /** Opus frames in one network packet */
    UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "1", ClampMax = "5"))
    int32 FramesPerPacket = 2;

    /** Largest packet that the host accepts, in bytes */
    UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "64"))
    int32 MaxPacketBytes = 2000;

    /** Most voice that waits to play, in seconds. Older voice drops, so the delay does not grow */
    UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "0.05"))
    float MaxQueuedSeconds = 0.4f;

    /** True opens the microphone when this machine controls the pawn: an open microphone. False waits for Set Transmitting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
    bool bTransmitWhenLocallyControlled = true;

    /** Multiplier of the heard voice level in the talk level. The heard voice is quieter than the microphone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Talk Level", meta = (ClampMin = "0.0"))
    float RemoteLevelGain = 10.f;

    /** How fast the talk level rises */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Talk Level", meta = (ClampMin = "0.1"))
    float TalkRiseSpeed = 18.f;

    /** How fast the talk level falls */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Talk Level", meta = (ClampMin = "0.1"))
    float TalkFallSpeed = 6.f;

    /** Seconds after the last packet when the voice level falls to zero */
    UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "0.05"))
    float LevelHoldSeconds = 0.15f;

private:
    /** Owner: listener handle on the microphone, or INDEX_NONE */
    int32 MicListener = INDEX_NONE;

    /** Owner: Opus encoder */
    TSharedPtr<class IVoiceEncoder> Encoder;

    /** Owner: voice-rate samples as 16-bit PCM bytes that wait for a full packet */
    TArray<uint8> PendingPcm;

    /** Owner: microphone samples that the next block still needs for its first output sample */
    TArray<float> ResampleTail;

    /** Owner: position of the next output sample inside the tail and the next block */
    double ResamplePosition = 0.0;

    /** Listener: Opus decoder */
    TSharedPtr<class IVoiceDecoder> Decoder;

    /** Listener: sound that plays the decoded voice */
    UPROPERTY(Transient)
    TObjectPtr<class USoundWaveProcedural> VoiceWave;

    /** Smooth talk level of this frame */
    float TalkLevel = 0.f;

    /** Owner: true after the automatic start of the microphone, so push to talk keeps control */
    bool bAutoTransmitDone = false;

    /** Listener: peak of the latest decoded packet */
    float LastPeak = 0.f;

    /** Listener: time of the latest decoded packet */
    float LastPacketTime = -100.f;
};
