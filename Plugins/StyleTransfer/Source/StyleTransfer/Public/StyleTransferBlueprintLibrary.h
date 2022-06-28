// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include "StyleTransferBlueprintLibrary.generated.h"

UCLASS()
class STYLETRANSFER_API UStyleTransferBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_UCLASS_BODY()
public:
    UFUNCTION( BlueprintCallable, Category="StyleTransfer" )
    static void SetNeuralNetwork( class UNeuralNetwork* InNNetwork );
    UFUNCTION( BlueprintCallable, Category="StyleTransfer" )
    static void ResetNeuralNetwork();
};
