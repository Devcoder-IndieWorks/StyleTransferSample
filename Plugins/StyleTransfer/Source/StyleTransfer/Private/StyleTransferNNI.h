// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include <CoreMinimal.h>
#include <NeuralNetwork.h>
#include "StyleTransferNNI.generated.h"

UCLASS() 
class UStyleTransferNNI : public UObject
{
    GENERATED_BODY()
public:
    void SetNeuralNetwork( UNeuralNetwork* InNNetwork );
    void RunModel( const TArray<float>& InImage, TArray<uint8>& OutResults, bool InShowLog );

private:
    UPROPERTY( Transient )
    UNeuralNetwork* NeuralNetwork = nullptr;
};
