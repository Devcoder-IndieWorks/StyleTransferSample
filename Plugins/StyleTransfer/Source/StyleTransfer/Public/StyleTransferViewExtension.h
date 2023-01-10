// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include <SceneViewExtension.h>

class STYLETRANSFER_API FStyleTransferViewExtension : public FSceneViewExtensionBase
{
public:
    FStyleTransferViewExtension( const FAutoRegister& InAutoRegister );

    static void SetupStyleTransfer( class UNeuralNetwork* InNNetwork, uint8 InPassId, bool InShowLog );
    static void ReleaseStyleTransfer();

public:
    // Begin ISceneViewExtension interface
    virtual void SetupViewFamily( FSceneViewFamily& InViewFamily ) override {}
    virtual void SetupView( FSceneViewFamily& InViewFamily, FSceneView& InView ) override {}
    virtual void BeginRenderViewFamily( FSceneViewFamily& InViewFamily ) override {}
    virtual void PreRenderViewFamily_RenderThread( FRHICommandListImmediate& InRHICmdList, FSceneViewFamily& InViewFamily ) override {}
    virtual void PreRenderView_RenderThread( FRHICommandListImmediate& InRHICmdList, FSceneView& InView ) override {}
    virtual bool IsActiveThisFrame_Internal( const FSceneViewExtensionContext& InContext ) const override;
    virtual void SubscribeToPostProcessingPass( EPostProcessingPass InPassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool InIsPassEnabled ) override;
    virtual int32 GetPriority() const override;
    // End ISceneViewExtension interface

    FScreenPassTexture AddAfterPass_RenderThread( FRDGBuilder& InGraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& InOutInputs );

private:
    void AddStyleTransferPass_RenderThread( FRDGBuilder& InGraphBuilder, FRDGTextureRef InSourceTexture );
    void CopyTextureFromGPUToCPU( FRHICommandListImmediate& InRHICmdList, FRHITexture2D* InGPUTexture );
    void CopyTextureFromCPUToGPU( FRHICommandListImmediate& InRHICmdList, FRHITexture2D* OutGPUTexture );
    void ResizeScreenImageToMatchModel();
    void ResizeModelImageToMatchScreen();
    void ApplyStyleTransfer();

private:
    bool ViewExtensionIsActive;

    int32 Width;
    int32 Height;

    TArray<FColor> RawImage;
    TArray<uint8> InputImageCPU;
    TArray<uint32> StylizedImageCPU;
    TArray<float> ModelInputImage;
    TArray<uint8> ModelOutputImage;
};
