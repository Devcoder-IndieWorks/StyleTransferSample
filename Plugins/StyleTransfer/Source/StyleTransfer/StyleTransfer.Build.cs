// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System.IO;

public class StyleTransfer : ModuleRules
{
	public StyleTransfer( ReadOnlyTargetRules Target ) : base( Target )
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// OpenCV 플러그인에서 경고가 발생되지 않도록 하기 위해 다음 내용이 추가됨.
		bEnableUndefinedIdentifierWarnings = false;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		
		// Renderer의 Private 폴더에서 헤더파일을 include하기 위해 폴더 경로가 추가됨.
		PrivateIncludePaths.AddRange( new string[] { Path.Combine( EngineDirectory, "Source/Runtime/Renderer/Private" ) } );
		
		PublicDependencyModuleNames.AddRange( new string[] { "Core", "CoreUObject", "Engine" } );
		PrivateDependencyModuleNames.AddRange( new string[] { "Renderer", "RenderCore", "RHI", "RHICore", 
		    "D3D12RHI", "OpenCV", "OpenCVHelper", "NeuralNetworkInference" } );
	}
}