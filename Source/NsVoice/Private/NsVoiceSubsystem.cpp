// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVoiceSubsystem.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UNsVoiceSubsystem* UNsVoiceSubsystem::Get(const UObject* const WorldContext)
{
    const UGameInstance* const GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
    return GameInstance != nullptr ? GameInstance->GetSubsystem<UNsVoiceSubsystem>() : nullptr;
}

void UNsVoiceSubsystem::SetPlayerVolume(const FString& PlayerName, const float Volume)
{
    PlayerVolumes.Add(PlayerName, FMath::Clamp(Volume, 0.f, 2.f));
}

float UNsVoiceSubsystem::GetPlayerVolume(const FString& PlayerName) const
{
    const float* const Volume = PlayerVolumes.Find(PlayerName);
    return Volume != nullptr ? *Volume : 1.f;
}
