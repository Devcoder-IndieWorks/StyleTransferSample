#include "StyleTransferNNI.h"
#include "StyleTransferLog.h"

static uint8 __FloatToColor( float InValue )
{
    return (uint8)( FMath::Clamp( InValue, 0.0f, 255.0f ) );
}

void UStyleTransferNNI::SetNeuralNetwork( UNeuralNetwork* InNNetwork )
{
    NeuralNetwork = InNNetwork;
}

void UStyleTransferNNI::RunModel( const TArray<float>& InImage, TArray<uint8>& OutResults, bool InShowLog )
{
    if ( ensure( NeuralNetwork != nullptr && NeuralNetwork->IsLoaded() ) ) {
        auto startSeconds = FPlatformTime::Seconds();

        // Running inference
        NeuralNetwork->SetInputFromArrayCopy( InImage );
        NeuralNetwork->Run();

        auto outputTensor = NeuralNetwork->GetOutputTensor().GetArrayCopy<float>();

        OutResults.Reset();
        int32 channelStride = InImage.Num() / 3;
        //OutResults.Reserve( channelStride * 4 );

        //for ( int32 i = 0; i < channelStride; i++ ) {
        //    OutResults.Add( __FloatToColor( outputTensor[ channelStride * 2 + i ] ) ); // B
        //    OutResults.Add( __FloatToColor( outputTensor[ channelStride + i ] ) );     // G
        //    OutResults.Add( __FloatToColor( outputTensor[ i ] ) );                     // R
        //}
        OutResults.SetNumZeroed( channelStride * 4 );
        ParallelFor( channelStride, [&]( int32 idx ) {
            const int32 INDEX = idx * 3;
            OutResults[ INDEX     ] = __FloatToColor( outputTensor[ channelStride * 2 + idx ] ); // B
            OutResults[ INDEX + 1 ] = __FloatToColor( outputTensor[ channelStride + idx ] );     // G
            OutResults[ INDEX + 2 ] = __FloatToColor( outputTensor[ idx ] );                     // R
        } );

        double secondElapsed = FPlatformTime::Seconds() - startSeconds;
        if ( InShowLog ) STYLE_LOG( Log, TEXT( "Neural Network Inference RunModel completed in %f" ), secondElapsed );
    }
}
