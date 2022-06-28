// Copyright Epic Games, Inc. All Rights Reserved.

#include "StyleTransferSample.h"
#include "StyleTransferViewExtension.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FStyleTransferSampleModule, StyleTransferSample, "StyleTransferSample" );

void FStyleTransferSampleModule::StartupModule()
{
    StyleTransferViewExtension = FSceneViewExtensions::NewExtension<FStyleTransferViewExtension>();
}
