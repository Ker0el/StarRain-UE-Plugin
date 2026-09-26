// Copyright Saeid Gholizade. All Rights Reserved. 2020

using UnrealBuildTool;

public class StarRain : ModuleRules
{
	public StarRain(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"AssetTools",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
					"PropertyEditor",
					"EditorStyle",
					"Projects",
					"MeshMergeUtilities",
					"EditorFramework",

					"ChaosCore",
					"Chaos",
					"PhysicsCore",
					"GeometryCollectionEngine"
				// ... add private dependencies that you statically link with here ...
			}
			);


		// ── UE 5.4 兼容性绕行 ────────────────────────────────────────────────
		// 5.4 的 Engine/Source/Runtime/Core/Public/Experimental/ConcurrentLinearAllocator.h
		// 第 31 行裸用了 __has_feature(address_sanitizer) —— 那是 Clang 的宏，MSVC 不认，
		// 报 C4668 / C4067 直接编译失败。5.5 已修复（补了 defined() 保护）。
		//
		// 触发条件：Visual Studio 装了 ASAN 组件 → <sanitizer/asan_interface.h> 存在 →
		// Platform.h 里的 PLATFORM_HAS_ASAN_INCLUDE 求值为 1 → 那段代码被编译。
		//
		// Platform.h 用的是 #ifndef 保护，所以在这里预先定义成 0 就能绕开，
		// 且只影响本模块，不动引擎源码。
		PublicDefinitions.Add("PLATFORM_HAS_ASAN_INCLUDE=0");
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
