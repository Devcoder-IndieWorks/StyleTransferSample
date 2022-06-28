// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include <CoreMinimal.h>

DECLARE_LOG_CATEGORY_EXTERN( LogStyleTrans, Log, All )

#define STYLE_LOG_CALLINFO                  ( FString( TEXT( "[" ) ) + FString( __FUNCTION__ ) + FString( TEXT( "(" ) ) + FString::FromInt( __LINE__ ) + FString( TEXT( ")" ) ) + FString( TEXT( "]" ) ) )
#define STYLE_LOG_CALLONLY( Verbosity )     UE_LOG( LogStyleTrans, Verbosity, TEXT( "%s" ), *STYLE_LOG_CALLINFO )
#define STYLE_LOG( Verbosity, Format, ... ) UE_LOG( LogStyleTrans, Verbosity, TEXT( "%s LOG: #### %s ####" ), *STYLE_LOG_CALLINFO, *FString::Printf( Format, ##__VA_ARGS__ ) )
