#include "StyleTransferBlueprintLibrary.h"
#include "StyleTransferViewExtension.h"
#include "StyleTransferNNI.h"

UStyleTransferBlueprintLibrary::UStyleTransferBlueprintLibrary( const FObjectInitializer& ObjectInitializer )
    : Super( ObjectInitializer )
{
}

void UStyleTransferBlueprintLibrary::SetNeuralNetwork( UNeuralNetwork* InNNetwork )
{
    FStyleTransferViewExtension::SetupStyleTransfer( InNNetwork );
}

void UStyleTransferBlueprintLibrary::ResetNeuralNetwork()
{
    FStyleTransferViewExtension::ReleaseStyleTransfer();
}
