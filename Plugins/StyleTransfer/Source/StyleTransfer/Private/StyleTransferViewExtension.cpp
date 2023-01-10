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
    static uint32 PassId = 0;
    static bool ShowLog = true;

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

void FStyleTransferViewExtension::SetupStyleTransfer( UNeuralNetwork* InNNetwork, uint8 InPassId, bool InShowLog )
{
    StyleTransferRealtime::StyleTransfer = NewObject<UStyleTransferNNI>();
    StyleTransferRealtime::StyleTransfer->AddToRoot();
    StyleTransferRealtime::StyleTransfer->SetNeuralNetwork( InNNetwork );
    StyleTransferRealtime::PassId = InPassId;
    StyleTransferRealtime::ShowLog = InShowLog;
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

int32 FStyleTransferViewExtension::GetPriority() const
{
    return MIN_int32;
}

bool FStyleTransferViewExtension::IsActiveThisFrame_Internal( const FSceneViewExtensionContext& InContext ) const
{
    return ViewExtensionIsActive;
}

void FStyleTransferViewExtension::SubscribeToPostProcessingPass( EPostProcessingPass InPassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool InIsPassEnabled )
{
    if ( !StyleTransferRealtime::IsActive )
        return;

    if ( (StyleTransferRealtime::PassId != (uint32)EPostProcessingPass::VisualizeDepthOfField) && !InIsPassEnabled ) {
        if ( StyleTransferRealtime::ShowLog ) STYLE_LOG( Warning, TEXT( "Disable input pass" ) );
        return;
    }

    if ( (uint32)InPassId == StyleTransferRealtime::PassId )
        InOutPassCallbacks.Add( FAfterPassCallbackDelegate::CreateRaw( this, &FStyleTransferViewExtension::AddAfterPass_RenderThread ) );
}

FScreenPassTexture FStyleTransferViewExtension::AddAfterPass_RenderThread( FRDGBuilder& InGraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& InOutInputs )
{
    RDG_EVENT_SCOPE( InGraphBuilder, "StyleTransfer_AddAfterPass" );
    //auto ddsFilename = FString::Printf( TEXT( "After%02dTonemap" ), EPostProcessingPass::Tonemap );

    FScreenPassTexture screenpassTexture = InOutInputs.OverrideOutput;
    FRDGTextureRef sourceTexture = screenpassTexture.Texture;
    if ( !screenpassTexture.IsValid() ) {
        screenpassTexture = InOutInputs.GetInput( EPostProcessMaterialInput::SceneColor );
        sourceTexture = screenpassTexture.Texture;
        if ( StyleTransferRealtime::ShowLog ) STYLE_LOG( Log, TEXT( "EPostProcessMaterialInput::SceneColor Texture" ) );
    }

    AddStyleTransferPass_RenderThread( InGraphBuilder, sourceTexture );

    return MoveTemp(screenpassTexture);
}

BEGIN_SHADER_PARAMETER_STRUCT( FStyleTransferPassParameters, )
RDG_TEXTURE_ACCESS( Source, ERHIAccess::CPURead )
END_SHADER_PARAMETER_STRUCT()
void FStyleTransferViewExtension::AddStyleTransferPass_RenderThread( FRDGBuilder& InGraphBuilder, FRDGTextureRef InSourceTexture )
{
    if ( InSourceTexture == nullptr ) {
        if ( StyleTransferRealtime::ShowLog ) STYLE_LOG( Warning, TEXT( "Skipping nullptr texture." ) );
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
    auto startSeconds = FPlatformTime::Seconds();

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
    //InputImageCPU.Reserve( PIXEL_COUNT * 3 );
    //for ( int32 i = 0; i < RawImage.Num(); i++ ) {
    //    const auto& pixel = RawImage[ i ];
    //    InputImageCPU.Add( pixel.R );
    //    InputImageCPU.Add( pixel.G );
    //    InputImageCPU.Add( pixel.B );
    //}
    InputImageCPU.SetNumZeroed( PIXEL_COUNT * 3 );
    ParallelFor( RawImage.Num(), [&]( int32 idx ) {
        const int32 INDEX = idx * 3;
        const auto& PIXEL = RawImage[ idx ];
        InputImageCPU[ INDEX     ] = PIXEL.R;
        InputImageCPU[ INDEX + 1 ] = PIXEL.G;
        InputImageCPU[ INDEX + 2 ] = PIXEL.B;
    } );

    double secondElapsed = FPlatformTime::Seconds() - startSeconds;
    if ( StyleTransferRealtime::ShowLog ) STYLE_LOG( Log, TEXT( "CopyTextureFromGPUToCPU completed in %f" ), secondElapsed );
}

void FStyleTransferViewExtension::ResizeScreenImageToMatchModel()
{
    auto startSeconds = FPlatformTime::Seconds();

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
    //ModelInputImage.Reserve( INPUT_SIZE );
    //for ( size_t ch = 0; ch < 3; ++ch ) {
    //    for ( size_t i = ch; i < INPUT_SIZE; i += 3 )
    //        ModelInputImage.Add( vec[ i ] );
    //}
    ModelInputImage.SetNumZeroed( INPUT_SIZE );
    for ( size_t ch = 0; ch < 3; ++ch ) {
        const int BLOCK_SIZE = INPUT_SIZE / 3;
        ParallelFor( BLOCK_SIZE, [&]( int32 idx ) {
            const int INDEX = (idx * 3) + ch;
            const int STRIDE = ch * BLOCK_SIZE;
            ModelInputImage[ idx + STRIDE ] = vec[ INDEX ];
        } );
    }

    double secondElapsed = FPlatformTime::Seconds() - startSeconds;
    if ( StyleTransferRealtime::ShowLog ) STYLE_LOG( Log, TEXT( "ResizeScreenImageToMatchModel completed in %f" ), secondElapsed );
}

void FStyleTransferViewExtension::ApplyStyleTransfer()
{
    // 종료시 "LogUObjectBase: Error: 'this' pointer is invalid." 메세지를 출력함.
    // : StyleTransfer가 Main Thread에서 제거되기 때문.
    if ( StyleTransferRealtime::StyleTransfer->IsValidLowLevelFast() ) {
        ModelOutputImage.Reset();
        StyleTransferRealtime::StyleTransfer->RunModel( ModelInputImage, ModelOutputImage, StyleTransferRealtime::ShowLog );
    }
}

void FStyleTransferViewExtension::ResizeModelImageToMatchScreen()
{
    auto startSeconds = FPlatformTime::Seconds();

    if ( ModelOutputImage.Num() == 0 )
        return;

    cv::Mat resultImage( 224, 224, CV_8UC3, ModelOutputImage.GetData() );
    cv::resize( resultImage, resultImage, cv::Size( Width, Height ) );

    const uint8* rawPixel = resultImage.data;
    const int32 PIXEL_COUNT = Width * Height;
    StylizedImageCPU.Reset();
    //StylizedImageCPU.Reserve( PIXEL_COUNT );

    //int32 y = 0;
    //for ( int32 i = 0; i < PIXEL_COUNT; i++ ) {
    //    uint8 r = rawPixel[ y++ ];
    //    uint8 g = rawPixel[ y++ ];
    //    uint8 b = rawPixel[ y++ ];
    //    uint32 color = (r << 22) | (g << 12) | (b << 2) | 3;
    //    StylizedImageCPU.Add( color );
    //}
    StylizedImageCPU.SetNumZeroed( PIXEL_COUNT );
    ParallelFor( PIXEL_COUNT, [&]( int32 idx ) {
        const int32 INDEX = idx * 3;
        uint8 r = rawPixel[ INDEX ];
        uint8 g = rawPixel[ INDEX + 1 ];
        uint8 b = rawPixel[ INDEX + 2 ];
        uint32 color = (r << 22) | (g << 12) | (b << 2) | 3;
        StylizedImageCPU[ idx ] = color;
    } );

    double secondElapsed = FPlatformTime::Seconds() - startSeconds;
    if ( StyleTransferRealtime::ShowLog ) STYLE_LOG( Log, TEXT( "ResizeModelImageToMatchScreen completed in %f" ), secondElapsed );
}

void FStyleTransferViewExtension::CopyTextureFromCPUToGPU( FRHICommandListImmediate& InRHICmdList, FRHITexture2D* OutGPUTexture )
{
    const FUpdateTextureRegion2D textureRegion2D( 0, 0, 0, 0, Width, Height );
    InRHICmdList.UpdateTexture2D( OutGPUTexture, 0, textureRegion2D, Width * 4, (const uint8*)StylizedImageCPU.GetData() );
}
