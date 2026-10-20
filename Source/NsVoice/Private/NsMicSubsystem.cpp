// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsMicSubsystem.h"

#include "Engine/GameInstance.h"
#include "NsVoiceModule.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeLock.h"

void UNsMicSubsystem::Deinitialize()
{
    CloseStream();
    Super::Deinitialize();
}

UNsMicSubsystem* UNsMicSubsystem::Get(const UObject* WorldContext)
{
    const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
    return GameInstance != nullptr ? GameInstance->GetSubsystem<UNsMicSubsystem>() : nullptr;
}

TArray<FNsMicDevice> UNsMicSubsystem::GetDevices()
{
    TArray<Audio::FCaptureDeviceInfo> Infos;
    Audio::FAudioCapture Probe;
    Probe.GetCaptureDevicesAvailable(Infos);
    TArray<FNsMicDevice> Devices;
    for (const Audio::FCaptureDeviceInfo& Info : Infos)
    {
        Devices.Add({ Info.DeviceId, Info.DeviceName });
    }
    return Devices;
}

FText UNsMicSubsystem::GetDeviceName(const FString& DeviceId)
{
    for (const FNsMicDevice& Device : GetDevices())
    {
        if (!DeviceId.IsEmpty() && Device.Id == DeviceId)
        {
            return FText::FromString(Device.Name);
        }
    }
    return FText::GetEmpty();
}

int32 UNsMicSubsystem::OpenListener(const float MaxSeconds)
{
    if (!Capture.IsStreamOpen() && !OpenStream())
    {
        return INDEX_NONE;
    }
    const int32 Handle = NextListener++;
    ListenerSeconds.Add(Handle, MaxSeconds);
    FScopeLock Lock(&ListenerMutex);
    Listeners.Add(Handle).MaxSamples = FMath::CeilToInt(MaxSeconds * SampleRate);
    return Handle;
}

void UNsMicSubsystem::CloseListener(const int32 Listener)
{
    ListenerSeconds.Remove(Listener);
    bool bLast = false;
    {
        FScopeLock Lock(&ListenerMutex);
        Listeners.Remove(Listener);
        bLast = Listeners.IsEmpty();
    }
    if (bLast)
    {
        CloseStream();
    }
}

bool UNsMicSubsystem::DrainSamples(const int32 Listener, TArray<float>& Out)
{
    FScopeLock Lock(&ListenerMutex);
    FListener* Entry = Listeners.Find(Listener);
    if (Entry == nullptr)
    {
        return false;
    }
    Out.Append(Entry->Samples);
    Entry->Samples.Reset();
    return true;
}

float UNsMicSubsystem::GetLevel() const
{
    return Capture.IsStreamOpen() ? FMath::Clamp(Peak.load() * Gain, 0.f, 1.f) : 0.f;
}

void UNsMicSubsystem::SetDeviceId(const FString& InDeviceId)
{
    if (DeviceId == InDeviceId)
    {
        return;
    }
    DeviceId = InDeviceId;
    ReopenDevice();
}

void UNsMicSubsystem::ReopenDevice()
{
    if (!Capture.IsStreamOpen())
    {
        return;
    }
    CloseStream();
    if (!OpenStream())
    {
        return;
    }

    // The new device can have another sample rate, so every queue starts again at the new size
    FScopeLock Lock(&ListenerMutex);
    for (TPair<int32, FListener>& Pair : Listeners)
    {
        Pair.Value.Samples.Reset();
        Pair.Value.MaxSamples = FMath::CeilToInt(ListenerSeconds.FindRef(Pair.Key) * SampleRate);
    }
}

bool UNsMicSubsystem::OpenStream()
{
    // A device that is not connected now falls back to the Windows default
    TArray<Audio::FCaptureDeviceInfo> Infos;
    Capture.GetCaptureDevicesAvailable(Infos);
    Audio::FAudioCaptureDeviceParams Params;
    Params.DeviceIndex = DeviceId.IsEmpty() ? Audio::DefaultDeviceIndex
        : Infos.IndexOfByPredicate([this](const Audio::FCaptureDeviceInfo& Info) { return Info.DeviceId == DeviceId; });
    if (!DeviceId.IsEmpty() && Params.DeviceIndex == INDEX_NONE)
    {
        UE_LOG(LogNsVoice, Warning, TEXT("The picked microphone %s is not connected. The game uses the Windows default microphone."), *DeviceId);
    }

    // The audio thread calls this function, so it holds only a weak pointer
    TWeakObjectPtr<UNsMicSubsystem> WeakThis(this);
    Audio::FOnAudioCaptureFunction OnCapture = [WeakThis](const void* Audio, const int32 NumFrames, const int32 NumChannels, int32, double, bool)
    {
        if (UNsMicSubsystem* Mic = WeakThis.Get())
        {
            Mic->HandleAudio(static_cast<const float*>(Audio), NumFrames, NumChannels);
        }
    };
    Audio::FCaptureDeviceInfo Info;
    if (!Capture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024) || !Capture.GetCaptureDeviceInfo(Info, Params.DeviceIndex))
    {
        UE_LOG(LogNsVoice, Warning, TEXT("No microphone opened. Voice chat stays off."));
        Capture.CloseStream();
        return false;
    }
    SampleRate = Info.PreferredSampleRate;
    Capture.StartStream();
    return true;
}

void UNsMicSubsystem::CloseStream()
{
    if (Capture.IsStreamOpen())
    {
        Capture.StopStream();
        Capture.CloseStream();
    }
    SampleRate = 0;
    Peak.store(0.f);
}

void UNsMicSubsystem::HandleAudio(const float* Audio, const int32 NumFrames, const int32 NumChannels)
{
    if (Audio == nullptr || NumChannels <= 0)
    {
        return;
    }
    float BlockPeak = 0.f;
    TArray<float, TInlineAllocator<1024>> Mono;
    Mono.SetNumUninitialized(NumFrames);
    for (int32 Frame = 0; Frame < NumFrames; ++Frame)
    {
        float Sum = 0.f;
        for (int32 Channel = 0; Channel < NumChannels; ++Channel)
        {
            Sum += Audio[Frame * NumChannels + Channel];
        }
        Mono[Frame] = Sum / NumChannels;
        BlockPeak = FMath::Max(BlockPeak, FMath::Abs(Mono[Frame]));
    }
    Peak.store(BlockPeak);
    FScopeLock Lock(&ListenerMutex);
    for (TPair<int32, FListener>& Pair : Listeners)
    {
        const int32 Room = Pair.Value.MaxSamples - Pair.Value.Samples.Num();
        Pair.Value.Samples.Append(Mono.GetData(), FMath::Clamp(Room, 0, Mono.Num()));
    }
}
