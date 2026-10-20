// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AudioCaptureCore.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NsMicSubsystem.generated.h"

/** One microphone that the player can pick */
struct NSVOICE_API FNsMicDevice
{
    /** Stable ID of the device. The options save it */
    FString Id;

    /** Name that the player reads */
    FString Name;
};

/**
 * Owns the one microphone stream of this PC: the device that the game picked with Set Device Id, or the Windows default
 * Voice chat and a level meter each open a listener. The stream runs while one listener is open
 * Each listener gets its own queue of mono samples, which it reads on the game thread
 */
UCLASS()
class NSVOICE_API UNsMicSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin USubsystem Interface
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    /** Returns the microphone subsystem of the game instance of a world context, or null */
    static UNsMicSubsystem* Get(const UObject* WorldContext);

    /** Returns the microphones of this PC */
    static TArray<FNsMicDevice> GetDevices();

    /** Returns the name of the picked microphone, or an empty text for the Windows default */
    static FText GetDeviceName(const FString& DeviceId);

    /** Opens a listener that keeps up to MaxSeconds of samples. Returns its handle, or INDEX_NONE when no microphone opens */
    int32 OpenListener(float MaxSeconds);

    /** Closes a listener. The stream closes with the last listener */
    void CloseListener(int32 Listener);

    /** Moves the samples of a listener into Out. Returns false for a closed listener */
    bool DrainSamples(int32 Listener, TArray<float>& Out);

    /** Returns the sample rate of the open stream, or 0 */
    int32 GetSampleRate() const { return SampleRate; }

    /** Returns the latest loudness from 0 to 1, after the gain */
    float GetLevel() const;

    /** Picks the microphone by its device ID. Empty picks the Windows default. An open stream opens again on the new device */
    void SetDeviceId(const FString& InDeviceId);

    /** Returns the ID of the picked microphone, or empty for the Windows default */
    const FString& GetDeviceId() const { return DeviceId; }

    /** Sets the gain of the microphone, from 0 to 4. The voice chat sends the samples after the gain */
    void SetGain(float InGain) { Gain = FMath::Clamp(InGain, 0.f, 4.f); }

    /** Returns the gain of the microphone */
    float GetGain() const { return Gain; }

    /** Opens the stream again on the picked device. Listeners stay open */
    void ReopenDevice();

private:
    /** Opens the stream on the picked device */
    bool OpenStream();

    /** Closes the stream */
    void CloseStream();

    /** Audio thread: mixes a block to mono and adds it to every listener queue */
    void HandleAudio(const float* Audio, int32 NumFrames, int32 NumChannels);

// Variables
private:
    /** Sample queue of one listener */
    struct FListener
    {
        /** Mono samples that the listener has not read */
        TArray<float> Samples;

        /** Most samples in the queue. New samples are dropped when it is full */
        int32 MaxSamples = 0;
    };

    /** Capture stream of the microphone */
    Audio::FAudioCapture Capture;

    /** Open listeners by handle */
    TMap<int32, FListener> Listeners;

    /** Protects the listener queues, which the audio thread writes */
    mutable FCriticalSection ListenerMutex;

    /** Handle of the next listener */
    int32 NextListener = 0;

    /** Sample rate of the open stream */
    int32 SampleRate = 0;

    /** Max seconds of each listener, kept to size new queues after a device change */
    TMap<int32, float> ListenerSeconds;

    /** ID of the picked microphone, or empty for the Windows default */
    FString DeviceId;

    /** Gain of the microphone */
    float Gain = 1.5f;

    /** Peak of the latest audio block, written on the audio thread */
    std::atomic<float> Peak { 0.f };
};
