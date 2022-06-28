// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FStyleTransferSampleModule : public IModuleInterface
{
public:
    void StartupModule() override;

private:
    TSharedPtr<class FSceneViewExtensionBase> StyleTransferViewExtension;
};

