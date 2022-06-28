// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include <GameFramework/Actor.h>
#include "StyleTransferActorBase.generated.h"

UCLASS()
class STYLETRANSFER_API AStyleTransferActorBase : public AActor
{
    GENERATED_UCLASS_BODY()
public:
    void BeginPlay() override;
    void EndPlay( const EEndPlayReason::Type InEndPlayReason ) override;

private:
    UPROPERTY( EditAnywhere, Category=StyleTransfer, meta=( AllowPrivateAccess="true" ) )
    class UNeuralNetwork* NeuralNetwork;
};
