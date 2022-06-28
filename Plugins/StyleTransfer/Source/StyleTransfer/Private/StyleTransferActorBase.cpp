#include "StyleTransferActorBase.h"
#include "StyleTransferBlueprintLibrary.h"
#include <NeuralNetwork.h>
#include <Engine/Engine.h>

AStyleTransferActorBase::AStyleTransferActorBase( const FObjectInitializer& ObjectInitializer )
    : Super( ObjectInitializer )
{
}

void AStyleTransferActorBase::BeginPlay()
{
    Super::BeginPlay();

    if ( GEngine != nullptr ) {
        GEngine->Exec( GetWorld(), TEXT( "r.StyleTransferRealtime.Enable 1" ) );

        if ( ensure( NeuralNetwork != nullptr ) )
            UStyleTransferBlueprintLibrary::SetNeuralNetwork( NeuralNetwork );
    }
}

void AStyleTransferActorBase::EndPlay( const EEndPlayReason::Type InEndPlayReason )
{
    if ( GEngine != nullptr ) {
        GEngine->Exec( GetWorld(), TEXT( "r.StyleTransferRealtime.Enable 0" ) );
        UStyleTransferBlueprintLibrary::ResetNeuralNetwork();
    }

    Super::EndPlay( InEndPlayReason );
}
