#include "StyleTransferBlueprintLibrary.h"
#include "StyleTransferViewExtension.h"
#include "StyleTransferNNI.h"

UStyleTransferBlueprintLibrary::UStyleTransferBlueprintLibrary( const FObjectInitializer& ObjectInitializer )
    : Super( ObjectInitializer )
{
}

void UStyleTransferBlueprintLibrary::SetNeuralNetwork( UNeuralNetwork* InNNetwork, uint8 InPassId, bool InShowLog )
{
    FStyleTransferViewExtension::SetupStyleTransfer( InNNetwork, InPassId, InShowLog );
}

void UStyleTransferBlueprintLibrary::ResetNeuralNetwork()
{
    FStyleTransferViewExtension::ReleaseStyleTransfer();
}
