// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include <GameFramework/Actor.h>
#include "StyleTransferActorBase.generated.h"

UENUM( BlueprintType ) 
enum class ESTPostProcessingPass : uint8
{
    ST_SSInput               UMETA( DisplayName="SSInput" ),
    ST_MotionBlure           UMETA( DisplayName="MotionBlur" ),
    ST_Tonemap               UMETA( DisplayName="Tonemap" ),
    ST_FXAA                  UMETA( DisplayName="FXAA" ),
    ST_VisualizeDepthOfField UMETA( DisplayName="VisualizeDepthOfField" ),
    ST_MAX                   UMETA( Hidden )
};

//-----------------------------------------------------------------------------

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
    UPROPERTY( EditAnywhere, Category=StyleTransfer, meta=( AllowPrivateAccess="true" ) )
    ESTPostProcessingPass Pass = ESTPostProcessingPass::ST_VisualizeDepthOfField;
    UPROPERTY( EditAnywhere, Category=StyleTransfer )
    bool ShowLog = true;
};
