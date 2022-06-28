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

void UStyleTransferNNI::RunModel( const TArray<float>& InImage, TArray<uint8>& OutResults )
{
    if ( ensure( NeuralNetwork != nullptr && NeuralNetwork->IsLoaded() ) ) {
        // Running inference
        NeuralNetwork->SetInputFromArrayCopy( InImage );
        NeuralNetwork->Run();
        STYLE_LOG( Log, TEXT( "Neural Network Inference complete." ) );

        auto outputTensor = NeuralNetwork->GetOutputTensor().GetArrayCopy<float>();

        OutResults.Reset();
        int32 channelStride = InImage.Num() / 3;
        OutResults.Reserve( channelStride * 4 );

        for ( int32 i = 0; i < channelStride; i++ ) {
            OutResults.Add( __FloatToColor( outputTensor[ channelStride * 2 + i ] ) ); // B
            OutResults.Add( __FloatToColor( outputTensor[ channelStride + i ] ) );     // G
            OutResults.Add( __FloatToColor( outputTensor[ i ] ) );                     // R
        }
    }
}
