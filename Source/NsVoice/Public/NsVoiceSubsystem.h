// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NsVoiceSubsystem.generated.h"

/**
 * Volumes of the voices that this machine hears: one volume for every voice, and one volume for each player
 * The game sets them from its options. The voice chat component reads them each frame
 */
UCLASS()
class NSVOICE_API UNsVoiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

// Functions
public:
    /** Returns the voice subsystem of the game instance of a world context, or null */
    static UNsVoiceSubsystem* Get(const UObject* WorldContext);

    /** Sets the volume of every voice, from 0 to 2 */
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void SetVoiceVolume(float Volume) { VoiceVolume = FMath::Clamp(Volume, 0.f, 2.f); }

    /** Returns the volume of every voice */
    UFUNCTION(BlueprintPure, Category = "Voice")
    float GetVoiceVolume() const { return VoiceVolume; }

    /** Sets the voice volume of one player, by player name, from 0 to 2. It lasts for the session */
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void SetPlayerVolume(const FString& PlayerName, float Volume);

    /** Returns the voice volume of one player. A player without a volume gets 1 */
    UFUNCTION(BlueprintPure, Category = "Voice")
    float GetPlayerVolume(const FString& PlayerName) const;

// Variables
private:
    /** Volume of every voice */
    float VoiceVolume = 1.f;

    /** Voice volume of each player, by name */
    TMap<FString, float> PlayerVolumes;
};
