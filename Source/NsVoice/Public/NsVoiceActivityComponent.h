// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsVoiceActivityComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnNsSpeakingChanged, bool /* bSpeaking */);

/**
 * Tells every machine when a player speaks
 * The owner reads its microphone level and sends the speech state to the host. The host replicates it to the other players
 * Game code listens to On Speaking Changed, for example a cockpit light or an NPC that turns to the speaker
 */
UCLASS(ClassGroup = (NsVoice))
class NSVOICE_API UNsVoiceActivityComponent : public UActorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVoiceActivityComponent();

    //~ Begin UActorComponent Interface
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End UActorComponent Interface

    /** Returns true while this player speaks */
    bool IsSpeaking() const { return bSpeaking; }

    /** Owner: gives the microphone level from 0 to 1 while the microphone is open */
    void SetLocalMicLevel(float Level);

private:
    /** Owner: tells the host that speech starts or stops */
    UFUNCTION(Server, Reliable)
    void ServerSetSpeaking(bool bNewSpeaking);

    /** Shows a new speech state on this machine */
    UFUNCTION()
    void OnRep_Speaking();

    /** Returns the owning pawn */
    class APawn* GetPawn() const;

// Variables
public:
    /** Microphone level that counts as speech */
    UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float SpeechLevel = 0.12f;

    /** Seconds that speech lasts after the level drops, so short pauses do not stop it */
    UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "0.0"))
    float SpeechHoldSeconds = 0.4f;

    /** Fires on every machine when this player starts or stops speaking */
    FOnNsSpeakingChanged OnSpeakingChanged;

private:
    /** True while this player speaks */
    UPROPERTY(ReplicatedUsing = OnRep_Speaking)
    bool bSpeaking = false;

    /** Owner: time of the last loud microphone level */
    float LastLoudTime = -100.f;

    /** Speech state that this machine showed last */
    bool bShownSpeaking = false;
};
