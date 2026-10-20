// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVoiceActivityComponent.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

UNsVoiceActivityComponent::UNsVoiceActivityComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UNsVoiceActivityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UNsVoiceActivityComponent, bSpeaking, COND_SkipOwner);
}

void UNsVoiceActivityComponent::SetLocalMicLevel(const float Level)
{
    if (Level >= SpeechLevel)
    {
        LastLoudTime = GetWorld()->GetTimeSeconds();
    }
}

void UNsVoiceActivityComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    const APawn* const Character = GetPawn();
    if (!IsValid(Character) || !Character->IsLocallyControlled())
    {
        return;
    }

    // The owner decides when speech starts and stops. Short pauses inside a sentence do not stop it
    const bool bLoud = GetWorld()->GetTimeSeconds() - LastLoudTime <= SpeechHoldSeconds;
    if (bLoud != bSpeaking)
    {
        bSpeaking = bLoud;
        OnRep_Speaking();
        ServerSetSpeaking(bLoud);
    }
}

void UNsVoiceActivityComponent::ServerSetSpeaking_Implementation(const bool bNewSpeaking)
{
    bSpeaking = bNewSpeaking;
    OnRep_Speaking();
}

void UNsVoiceActivityComponent::OnRep_Speaking()
{
    if (bShownSpeaking == bSpeaking)
    {
        return;
    }
    bShownSpeaking = bSpeaking;
    OnSpeakingChanged.Broadcast(bSpeaking);
}

APawn* UNsVoiceActivityComponent::GetPawn() const
{
    return Cast<APawn>(GetOwner());
}
