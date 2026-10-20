// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVoiceChatComponent.h"

#include "Interfaces/VoiceCodec.h"
#include "Misc/AutomationTest.h"
#include "VoiceModule.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNsVoiceCodecTest, "NsVoice.Codec",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNsVoiceCodecTest::RunTest(const FString& Parameters)
{
    // The voice chat sends Opus packets of whole frames. One packet of a loud tone must come back as a loud tone of the same length
    const TSharedPtr<IVoiceEncoder> Encoder = FVoiceModule::Get().CreateVoiceEncoder(UNsVoiceChatComponent::VoiceSampleRate, 1, EAudioEncodeHint::VoiceEncode_Voice);
    const TSharedPtr<IVoiceDecoder> Decoder = FVoiceModule::Get().CreateVoiceDecoder(UNsVoiceChatComponent::VoiceSampleRate, 1);
    if (!TestTrue(TEXT("The Voice module gives an Opus encoder and decoder"), Encoder.IsValid() && Decoder.IsValid()))
    {
        return false;
    }
    constexpr int32 Frames = 2;
    constexpr int32 Samples = Frames * UNsVoiceChatComponent::FrameSamples;
    TArray<int16> Tone;
    for (int32 Index = 0; Index < Samples; ++Index)
    {
        Tone.Add(static_cast<int16>(16000.f * FMath::Sin(2.f * PI * 440.f * Index / UNsVoiceChatComponent::VoiceSampleRate)));
    }
    TArray<uint8> Packet;
    Packet.SetNumUninitialized(2000);
    uint32 PacketSize = Packet.Num();
    const int32 Left = Encoder->Encode(reinterpret_cast<const uint8*>(Tone.GetData()), Tone.Num() * sizeof(int16), Packet.GetData(), PacketSize);
    TestEqual(TEXT("Whole frames encode without a rest"), Left, 0);
    TestTrue(TEXT("The packet is small"), PacketSize > 0 && PacketSize < 400);

    TArray<int16> Decoded;
    Decoded.SetNumUninitialized(Samples * 4);
    uint32 DecodedSize = Decoded.Num() * sizeof(int16);
    Decoder->Decode(Packet.GetData(), PacketSize, reinterpret_cast<uint8*>(Decoded.GetData()), DecodedSize);
    TestEqual(TEXT("The packet decodes to the same number of samples"), static_cast<int32>(DecodedSize / sizeof(int16)), Samples);
    int32 Peak = 0;
    for (int32 Index = 0; Index < static_cast<int32>(DecodedSize / sizeof(int16)); ++Index)
    {
        Peak = FMath::Max(Peak, FMath::Abs(static_cast<int32>(Decoded[Index])));
    }
    TestTrue(TEXT("The decoded tone is still loud"), Peak > 8000);
    return true;
}

#endif
