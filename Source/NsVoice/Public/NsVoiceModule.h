// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Log category of NsVoice */
NSVOICE_API DECLARE_LOG_CATEGORY_EXTERN(LogNsVoice, Log, All);

/** Module of NsVoice. It has no startup work */
class FNsVoiceModule : public IModuleInterface
{
};
