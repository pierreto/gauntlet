// Fill out your copyright notice in the Description page of Project Settings.

#include "GripScripts/GS_Default.h"
#include "VRGripInterface.h"
#include "GripMotionControllerComponent.h"

UGS_Default::UGS_Default(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bIsActive = true;
	WorldTransformOverrideType = EGSTransformOverrideType::OverridesWorldTransform;
}

bool UGS_Default::GetWorldTransform_Implementation
(
	UGripMotionControllerComponent* GrippingController, 
	float DeltaTime, FTransform & WorldTransform, 
	const FTransform &ParentTransform, 
	FBPActorGripInformation &Grip, 
	AActor * actor, 
	UPrimitiveComponent * root, 
	bool bRootHasInterface, 
	bool bActorHasInterface, 
	bool bIsForTeleport
) 
{
	if (!GrippingController)
		return false;

	// Just simple transform setting
	WorldTransform = Grip.RelativeTransform * Grip.AdditionTransform * ParentTransform;

	// Check the grip lerp state, this it ouside of the secondary attach check below because it can change the result of it
	if ((Grip.SecondaryGripInfo.bHasSecondaryAttachment && Grip.SecondaryGripInfo.SecondaryAttachment) || Grip.SecondaryGripInfo.GripLerpState == EGripLerpState::EndLerp)
	{
		switch (Grip.SecondaryGripInfo.GripLerpState)
		{
		case EGripLerpState::StartLerp:
		case EGripLerpState::EndLerp:
		{
			if (Grip.SecondaryGripInfo.curLerp > 0.01f)
				Grip.SecondaryGripInfo.curLerp -= DeltaTime;
			else
			{
				if (Grip.SecondaryGripInfo.bHasSecondaryAttachment &&
					Grip.AdvancedGripSettings.SecondaryGripSettings.bUseSecondaryGripSettings &&
					Grip.AdvancedGripSettings.SecondaryGripSettings.SecondaryGripScaler < 1.0f)
				{
					Grip.SecondaryGripInfo.GripLerpState = EGripLerpState::ConstantLerp;
				}
				else
					Grip.SecondaryGripInfo.GripLerpState = EGripLerpState::NotLerping;
			}

		}break;
		case EGripLerpState::ConstantLerp:
		case EGripLerpState::NotLerping:
		default:break;
		}
	}

	// Handle the interp and multi grip situations, re-checking the grip situation here as it may have changed in the switch above.
	if ((Grip.SecondaryGripInfo.bHasSecondaryAttachment && Grip.SecondaryGripInfo.SecondaryAttachment) || Grip.SecondaryGripInfo.GripLerpState == EGripLerpState::EndLerp)
	{
		FTransform SecondaryTransform = Grip.RelativeTransform * ParentTransform;

		// Checking secondary grip type for the scaling setting
		ESecondaryGripType SecondaryType = ESecondaryGripType::SG_None;

		if (bRootHasInterface)
			SecondaryType = IVRGripInterface::Execute_SecondaryGripType(root);
		else if (bActorHasInterface)
			SecondaryType = IVRGripInterface::Execute_SecondaryGripType(actor);

		// If the grip is a custom one, skip all of this logic we won't be changing anything
		if (SecondaryType != ESecondaryGripType::SG_Custom)
		{
			// Variables needed for multi grip transform
			FVector BasePoint = ParentTransform.GetLocation(); // Get our pivot point
			const FTransform PivotToWorld = FTransform(FQuat::Identity, BasePoint);
			const FTransform WorldToPivot = FTransform(FQuat::Identity, -BasePoint);

			FVector frontLocOrig;
			FVector frontLoc;

			// Ending lerp out of a multi grip
			if (Grip.SecondaryGripInfo.GripLerpState == EGripLerpState::EndLerp)
			{
				frontLocOrig = (/*WorldTransform*/SecondaryTransform.TransformPosition(Grip.SecondaryGripInfo.SecondaryRelativeTransform.GetLocation())) - BasePoint;
				frontLoc = Grip.SecondaryGripInfo.LastRelativeLocation;

				frontLocOrig = FMath::Lerp(frontLoc, frontLocOrig, FMath::Clamp(Grip.SecondaryGripInfo.curLerp / Grip.SecondaryGripInfo.LerpToRate, 0.0f, 1.0f));
			}
			else // Is in a multi grip, might be lerping into it as well.
			{
				//FVector curLocation; // Current location of the secondary grip

				bool bPulledControllerLoc = false;
				if (GrippingController->bHasAuthority && Grip.SecondaryGripInfo.SecondaryAttachment->GetOwner() == GrippingController->GetOwner())
				{
					if (UGripMotionControllerComponent * OtherController = Cast<UGripMotionControllerComponent>(Grip.SecondaryGripInfo.SecondaryAttachment))
					{
						if (!OtherController->bUseWithoutTracking)
						{
							FVector Position;
							FRotator Orientation;
							float WorldToMeters = GetWorld() ? GetWorld()->GetWorldSettings()->WorldToMeters : 100.0f;
							if (OtherController->GripPollControllerState(Position, Orientation, WorldToMeters))
							{
								frontLoc = OtherController->CalcControllerComponentToWorld(Orientation, Position).GetLocation() - BasePoint;
								///*curLocation*/ frontLoc = OtherController->CalcNewComponentToWorld(FTransform(Orientation, Position)).GetLocation() - BasePoint;
								bPulledControllerLoc = true;
							}
						}
					}
				}

				if (!bPulledControllerLoc)
					/*curLocation*/ frontLoc = Grip.SecondaryGripInfo.SecondaryAttachment->GetComponentLocation() - BasePoint;

				frontLocOrig = (/*WorldTransform*/SecondaryTransform.TransformPosition(Grip.SecondaryGripInfo.SecondaryRelativeTransform.GetLocation())) - BasePoint;

				// Apply any smoothing settings and lerping in / constant lerping
				Default_ApplySmoothingAndLerp(Grip, frontLoc, frontLocOrig, DeltaTime);

				Grip.SecondaryGripInfo.LastRelativeLocation = frontLoc;
			}

			// Get any scaling addition from a scaling secondary grip type
			FVector Scaler = FVector(1.0f);
			Default_GetAnyScaling(Scaler, Grip, frontLoc, frontLocOrig, SecondaryType, SecondaryTransform);

			Grip.SecondaryGripInfo.SecondaryGripDistance = FVector::Dist(frontLocOrig, frontLoc);

			if (Grip.AdvancedGripSettings.SecondaryGripSettings.bUseSecondaryGripSettings && Grip.AdvancedGripSettings.SecondaryGripSettings.bUseSecondaryGripDistanceInfluence)
			{
				float rotScaler = 1.0f - FMath::Clamp((Grip.SecondaryGripInfo.SecondaryGripDistance - Grip.AdvancedGripSettings.SecondaryGripSettings.GripInfluenceDeadZone) / FMath::Max(Grip.AdvancedGripSettings.SecondaryGripSettings.GripInfluenceDistanceToZero, 1.0f), 0.0f, 1.0f);
				frontLoc = FMath::Lerp(frontLocOrig, frontLoc, rotScaler);
			}

			// Skip rot val for scaling only
			if (SecondaryType != ESecondaryGripType::SG_ScalingOnly)
			{
				// Get the rotation difference from the initial second grip
				FQuat rotVal = FQuat::FindBetweenVectors(frontLocOrig, frontLoc);

				// Rebase the world transform to the pivot point, add the rotation, remove the pivot point rebase
				WorldTransform = WorldTransform * WorldToPivot * FTransform(rotVal, FVector::ZeroVector, Scaler) * PivotToWorld;
			}
			else
			{
				// Rebase the world transform to the pivot point, add the scaler, remove the pivot point rebase
				WorldTransform = WorldTransform * WorldToPivot * FTransform(FQuat::Identity, FVector::ZeroVector, Scaler) * PivotToWorld;
			}
		}
	}
	return true;
}

void UGS_Default::Default_GetAnyScaling(FVector & Scaler, FBPActorGripInformation & Grip, FVector & frontLoc, FVector & frontLocOrig, ESecondaryGripType SecondaryType, FTransform & SecondaryTransform)
{
	if (Grip.SecondaryGripInfo.GripLerpState != EGripLerpState::EndLerp)
	{

		//float Scaler = 1.0f;
		if (SecondaryType == ESecondaryGripType::SG_FreeWithScaling_Retain || SecondaryType == ESecondaryGripType::SG_SlotOnlyWithScaling_Retain || SecondaryType == ESecondaryGripType::SG_ScalingOnly)
		{
			/*Grip.SecondaryScaler*/ Scaler = FVector(frontLoc.Size() / frontLocOrig.Size());
			//bRescalePhysicsGrips = true; // This is for the physics grips

			if (Grip.AdvancedGripSettings.SecondaryGripSettings.bUseSecondaryGripSettings && Grip.AdvancedGripSettings.SecondaryGripSettings.bLimitGripScaling)
			{
				// Get the total scale after modification
				// #TODO: convert back to singular float version? Can get Min() & Max() to convert the float to a range...think about it
				FVector WorldScale = /*WorldTransform*/SecondaryTransform.GetScale3D();
				FVector CombinedScale = WorldScale * Scaler;

				// Clamp to the minimum and maximum values
				CombinedScale.X = FMath::Clamp(CombinedScale.X, Grip.AdvancedGripSettings.SecondaryGripSettings.MinimumGripScaling.X, Grip.AdvancedGripSettings.SecondaryGripSettings.MaximumGripScaling.X);
				CombinedScale.Y = FMath::Clamp(CombinedScale.Y, Grip.AdvancedGripSettings.SecondaryGripSettings.MinimumGripScaling.Y, Grip.AdvancedGripSettings.SecondaryGripSettings.MaximumGripScaling.Y);
				CombinedScale.Z = FMath::Clamp(CombinedScale.Z, Grip.AdvancedGripSettings.SecondaryGripSettings.MinimumGripScaling.Z, Grip.AdvancedGripSettings.SecondaryGripSettings.MaximumGripScaling.Z);

				// Recreate in scaler form so that the transform chain below works as normal
				Scaler = CombinedScale / WorldScale;
			}
			//Scaler = Grip.SecondaryScaler;
		}
	}
}

void UGS_Default::Default_ApplySmoothingAndLerp(FBPActorGripInformation & Grip, FVector &frontLoc, FVector & frontLocOrig, float DeltaTime)
{
	if (Grip.SecondaryGripInfo.GripLerpState == EGripLerpState::StartLerp) // Lerp into the new grip to smooth the transition
	{
		if (Grip.AdvancedGripSettings.SecondaryGripSettings.SecondaryGripScaler < 1.0f)
		{
			FVector SmoothedValue = Grip.AdvancedGripSettings.SecondaryGripSettings.SmoothingOneEuro.RunFilterSmoothing(frontLoc, DeltaTime);

			frontLoc = FMath::Lerp(SmoothedValue, frontLoc, Grip.AdvancedGripSettings.SecondaryGripSettings.SecondaryGripScaler);
		}

		frontLocOrig = FMath::Lerp(frontLocOrig, frontLoc, FMath::Clamp(Grip.SecondaryGripInfo.curLerp / Grip.SecondaryGripInfo.LerpToRate, 0.0f, 1.0f));
	}
	else if (Grip.SecondaryGripInfo.GripLerpState == EGripLerpState::ConstantLerp) // If there is a frame by frame lerp
	{
		FVector SmoothedValue = Grip.AdvancedGripSettings.SecondaryGripSettings.SmoothingOneEuro.RunFilterSmoothing(frontLoc, DeltaTime);

		frontLoc = FMath::Lerp(SmoothedValue, frontLoc, Grip.AdvancedGripSettings.SecondaryGripSettings.SecondaryGripScaler);
	}
}