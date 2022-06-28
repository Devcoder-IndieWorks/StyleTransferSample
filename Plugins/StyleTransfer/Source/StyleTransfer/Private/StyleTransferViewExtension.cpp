#include "StyleTransferViewExtension.h"
#include "StyleTransferNNI.h"
#include "StyleTransferLog.h"

#include <Modules/ModuleManager.h>
#include <PostProcess/PostProcessMaterial.h>
#include <PostProcess/SceneRenderTargets.h>
#include <ScreenPass.h>

#include <PreOpenCVHeaders.h>
#include <OpenCVHelper.h>
#include <ThirdParty/OpenCV/include/opencv2/imgproc.hpp>
#include <ThirdParty/OpenCV/include/opencv2/highgui/highgui.hpp>
#include <ThirdParty/OpenCV/include/opencv2/core.hpp>
#include <PostOpenCVHeaders.h>

namespace StyleTransferRealtime
{
    static UStyleTransferNNI* StyleTransfer = nullptr;

    static int32 IsActive = 0;
    static FAutoConsoleVariableRef CVarStyleTransferIsActive(
        TEXT( "r.StyleTransferRealtime.Enable" ),
        IsActive,
        TEXT( "Allows an additinal rendering pass that will apply a neural style to the frame.\n" )
        TEXT( "=0:off (default), >0:on" ),
        ECVF_Cheat | ECVF_RenderThreadSafe
    );
}

//-----------------------------------------------------------------------------

void FStyleTransferViewExtension::SetupStyleTransfer( UNeuralNetwork* InNNetwork )
{
    StyleTransferRealtime::StyleTransfer = NewObject<UStyleTransferNNI>();
    StyleTransferRealtime::StyleTransfer->AddToRoot();
    StyleTransferRealtime::StyleTransfer->SetNeuralNetwork( InNNetwork );
}

void FStyleTransferViewExtension::ReleaseStyleTransfer()
{
    if ( StyleTransferRealtime::StyleTransfer->IsValidLowLevelFast() ) {
        StyleTransferRealtime::StyleTransfer->RemoveFromRoot();
        StyleTransferRealtime::StyleTransfer->ConditionalBeginDestroy();
        StyleTransferRealtime::StyleTransfer = nullptr;
    }
}

//-----------------------------------------------------------------------------

FStyleTransferViewExtension::FStyleTransferViewExtension( const FAutoRegister& InAutoRegister )
    : FSceneViewExtensionBase( InAutoRegister )
{
    ViewExtensionIsActive = GDynamicRHI->GetName() == FString( TEXT( "D3D12" ) );
}

bool FStyleTransferViewExtension::IsActiveThisFrame_Internal( const FSceneViewExtensionContext& InContext ) const
{
    return ViewExtensionIsActive;
}

void FStyleTransferViewExtension::SubscribeToPostProcessingPass( EPostProcessingPass InPassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool InIsPassEnabled )
{
    if ( !StyleTransferRealtime::IsActive )
        return;

    if ( !InIsPassEnabled )
        return;

    if ( InPassId == EPostProcessingPass::Tonemap )
        InOutPassCallbacks.Add( FAfterPassCallbackDelegate::CreateRaw( this, &FStyleTransferViewExtension::AfterTonemap_RenderThread ) );
}

FScreenPassTexture FStyleTransferViewExtension::AfterTonemap_RenderThread( FRDGBuilder& InGraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& InOutInputs )
{
    RDG_EVENT_SCOPE( InGraphBuilder, "StyleTransfer_AfterTonemap" );
    //auto ddsFilename = FString::Printf( TEXT( "After%02dTonemap" ), EPostProcessingPass::Tonemap );

    FRDGTextureRef sourceTexture = nullptr;
    FScreenPassTexture screenpassTexture;

    if ( InOutInputs.OverrideOutput.IsValid() ) {
        sourceTexture = InOutInputs.OverrideOutput.Texture;
        screenpassTexture = InOutInputs.OverrideOutput;
    }
    else {
        sourceTexture = InOutInputs.Textures[ (uint32)EPostProcessMaterialInput::SceneColor ].Texture;
        screenpassTexture = const_cast<FScreenPassTexture&>( InOutInputs.Textures[ (uint32)EPostProcessMaterialInput::SceneColor ] );
    }

    AddStyleTransferPass_RenderThread( InGraphBuilder, sourceTexture );

    return screenpassTexture;
}

BEGIN_SHADER_PARAMETER_STRUCT( FStyleTransferPassParameters, )
RDG_TEXTURE_ACCESS( Source, ERHIAccess::CPURead )
END_SHADER_PARAMETER_STRUCT()
void FStyleTransferViewExtension::AddStyleTransferPass_RenderThread( FRDGBuilder& InGraphBuilder, FRDGTextureRef InSourceTexture )
{
    if ( InSourceTexture == nullptr ) {
        STYLE_LOG( Warning, TEXT( "Skipping nullptr texture." ) );
        return;
    }

    auto parameters = InGraphBuilder.AllocParameters<FStyleTransferPassParameters>();
    parameters->Source = InSourceTexture;

    InGraphBuilder.AddPass(
        RDG_EVENT_NAME( "StyleTransferRealtime" ),
        parameters,
        ERDGPassFlags::Readback,
        [this, InSourceTexture]( FRHICommandListImmediate& RHICmdList ){
            if ( StyleTransferRealtime::StyleTransfer == nullptr )
                return;

            auto texture = InSourceTexture->GetRHI()->GetTexture2D();
            Width = texture->GetSizeX();
            Height = texture->GetSizeY();
            CopyTextureFromGPUToCPU( RHICmdList, texture );
            ResizeScreenImageToMatchModel();
            ApplyStyleTransfer();
            ResizeModelImageToMatchScreen();
            CopyTextureFromCPUToGPU( RHICmdList, texture );
        }
    );
}

void FStyleTransferViewExtension::CopyTextureFromGPUToCPU( FRHICommandListImmediate& InRHICmdList, FRHITexture2D* InGPUTexture )
{
    const int PIXEL_COUNT = Width * Height;
    struct FReadSurfaceContext
    {
        FRHITexture* BackBuffer;
        TArray<FColor>& OutDatas;
        FIntRect Rect;
        FReadSurfaceDataFlags Flags;
    };

    const FReadSurfaceContext surfaceContext = {
        InGPUTexture,
        RawImage,
        FIntRect( 0, 0, Width, Height ),
        FReadSurfaceDataFlags( RCM_UNorm, CubeFace_MAX )
    };

    InRHICmdList.ReadSurfaceData( surfaceContext.BackBuffer, surfaceContext.Rect, surfaceContext.OutDatas, surfaceContext.Flags );

    InputImageCPU.Reset();
    InputImageCPU.Reserve( PIXEL_COUNT * 3 );
    for ( int32 i = 0; i < RawImage.Num(); i++ ) {
        const auto& pixel = RawImage[ i ];
        InputImageCPU.Add( pixel.R );
        InputImageCPU.Add( pixel.G );
        InputImageCPU.Add( pixel.B );
    }
}

void FStyleTransferViewExtension::ResizeScreenImageToMatchModel()
{
    cv::Mat inputImage( Height, Width, CV_8UC3, InputImageCPU.GetData() );
    cv::Mat outputImage( 224, 224, CV_8UC3 );
    cv::resize( inputImage, outputImage, cv::Size( 224, 224 ) );

    // 1D array
    outputImage = outputImage.reshape( 1, 1 );

    // [0, 255] --> [0, 1]
    std::vector<float> vec;
    outputImage.convertTo( vec, CV_32FC1, 1.0 / 255.0 );

    // (height, width, channel) --> (channel, height, width)
    const int INPUT_SIZE = 224 * 224 * 3;
    ModelInputImage.Reset();
    ModelInputImage.Reserve( INPUT_SIZE );
    for ( size_t ch = 0; ch < 3; ++ch ) {
        for ( size_t i = ch; i < INPUT_SIZE; i += 3 )
            ModelInputImage.Add( vec[ i ] );
    }
}

void FStyleTransferViewExtension::ApplyStyleTransfer()
{
    if ( StyleTransferRealtime::StyleTransfer->IsValidLowLevelFast() ) {
        ModelOutputImage.Reset();
        StyleTransferRealtime::StyleTransfer->RunModel( ModelInputImage, ModelOutputImage );
    }
}

void FStyleTransferViewExtension::ResizeModelImageToMatchScreen()
{
    if ( ModelOutputImage.Num() == 0 )
        return;

    cv::Mat resultImage( 224, 224, CV_8UC3, ModelOutputImage.GetData() );
    cv::resize( resultImage, resultImage, cv::Size( Width, Height ) );

    const uint8* rawPixel = resultImage.data;
    const int32 PIXEL_COUNT = Width * Height;
    StylizedImageCPU.Reset();
    StylizedImageCPU.Reserve( PIXEL_COUNT );

    int32 y = 0;
    for ( int32 i = 0; i < PIXEL_COUNT; i++ ) {
        uint8 r = rawPixel[ y++ ];
        uint8 g = rawPixel[ y++ ];
        uint8 b = rawPixel[ y++ ];
        uint32 color = (r << 22) | (g << 12) | (b << 2) | 3;
        StylizedImageCPU.Add( color );
    }
}

void FStyleTransferViewExtension::CopyTextureFromCPUToGPU( FRHICommandListImmediate& InRHICmdList, FRHITexture2D* OutGPUTexture )
{
    const FUpdateTextureRegion2D textureRegion2D( 0, 0, 0, 0, Width, Height );
    InRHICmdList.UpdateTexture2D( OutGPUTexture, 0, textureRegion2D, Width * 4, (const uint8*)StylizedImageCPU.GetData() );
}
