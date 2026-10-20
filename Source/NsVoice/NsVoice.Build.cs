// Copyright (C) 2025 nulled.softworks. All rights reserved.

using UnrealBuildTool;

public class NsVoice : ModuleRules
{
    public NsVoice(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Engine modules and the engine plugin Audio Capture only. NsVoice depends on no studio plugin
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AudioCaptureCore",
            "Voice",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AudioCapture",
        });
    }
}
