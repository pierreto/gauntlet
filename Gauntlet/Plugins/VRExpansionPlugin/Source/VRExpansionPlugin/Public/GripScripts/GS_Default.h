#pragma once

#include "CoreMinimal.h"
#include "VRGripScriptBase.h"
#include "GameFramework/WorldSettings.h"
#include "GS_Default.generated.h"


/**
* The default grip transform logic for the motion controllers
*/
UCLASS(NotBlueprintable, ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UGS_Default : public UVRGripScriptBase
{
	GENERATED_BODY()
public:

	UGS_Default(const FObjectInitializer& ObjectInitializer);

	//virtual void BeginPlay_Implementation() override;
	virtual bool GetWorldTransform_Implementation(UGripMotionControllerComponent * GrippingController, float DeltaTime, FTransform & WorldTransform, const FTransform &ParentTransform, FBPActorGripInformation &Grip, AActor * actor, UPrimitiveComponent * root, bool bRootHasInterface, bool bActorHasInterface, bool bIsForTeleport) override;

	inline void Default_GetAnyScaling(FVector & Scaler, FBPActorGripInformation & Grip, FVector & frontLoc, FVector & frontLocOrig, ESecondaryGripType SecondaryType, FTransform & SecondaryTransform);
	inline void Default_ApplySmoothingAndLerp(FBPActorGripInformation & Grip, FVector &frontLoc, FVector & frontLocOrig, float DeltaTime);
};
