// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GripMotionControllerComponent.h"
#include "MotionControllerComponent.h"
#include "VRGripInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "VRInteractibleFunctionLibrary.h"
#include "PhysicsEngine/ConstraintInstance.h"

#include "PhysicsPublic.h"

#if WITH_PHYSX
#include "PhysXSupport.h"
#endif // WITH_PHYSX


#include "VRLeverComponent.generated.h"


UENUM(Blueprintable)
enum class EVRInteractibleLeverAxis : uint8
{
	Axis_X,
	Axis_Y,
	Axis_Z,
	Axis_XY,
	Axis_XZ
};

UENUM(Blueprintable)
enum class EVRInteractibleLeverEventType : uint8
{
	LeverPositive,
	LeverNegative
};

UENUM(Blueprintable)
enum class EVRInteractibleLeverReturnType : uint8
{
	/** Stays in place on drop */
	Stay,

	/** Returns to zero on drop (lerps) */
	ReturnToZero,

	/** Lerps to closest max (only works with X/Y/Z axis levers) */
	LerpToMax,

	/** Lerps to closest max if over the toggle threshold (only works with X/Y/Z axis levers) */
	LerpToMaxIfOverThreshold,

	/** Retains momentum on release (only works with X/Y/Z axis levers) */
	RetainMomentum
};

/** Delegate for notification when the lever state changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FVRLeverStateChangedSignature, bool, LeverStatus, EVRInteractibleLeverEventType, LeverStatusType, float, LeverAngleAtTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVRLeverFinishedLerpingSignature, float, FinalAngle);

/**
* A Lever component, can act like a lever, door, wheel, joystick.
* If set to replicates it will replicate its values to the clients.
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRLeverComponent : public UStaticMeshComponent, public IVRGripInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UVRLeverComponent(const FObjectInitializer& ObjectInitializer);


	~UVRLeverComponent();

	// Call to use an object
	UPROPERTY(BlueprintAssignable, Category = "VRLeverComponent")
		FVRLeverStateChangedSignature OnLeverStateChanged;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Lever State Changed"))
		void ReceiveLeverStateChanged(bool LeverStatus, EVRInteractibleLeverEventType LeverStatusType, float LeverAngleAtTime);

	UPROPERTY(BlueprintAssignable, Category = "VRLeverComponent")
		FVRLeverFinishedLerpingSignature OnLeverFinishedLerping;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Lever Finished Lerping"))
		void ReceiveLeverFinishedLerping(float LeverFinalAngle);

	// Primary axis angle only
	UPROPERTY(BlueprintReadOnly, Category = "VRLeverComponent")
		float CurrentLeverAngle;

	// Bearing Direction, for X/Y is their signed direction, for XY mode it is an actual 2D directional vector
	UPROPERTY(BlueprintReadOnly, Category = "VRLeverComponent")
		FVector CurrentLeverForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		bool bIsPhysicsLever;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		bool bUngripAtTargetRotation;

	// Rotation axis to use, XY is combined X and Y, only LerpToZero and PositiveLimits work with this mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		EVRInteractibleLeverAxis LeverRotationAxis;

	// The percentage of the angle at witch the lever will toggle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
		float LeverTogglePercentage;

	// The max angle of the lever in the positive direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent", meta = (ClampMin = "0.0", ClampMax = "179.8", UIMin = "0.0", UIMax = "180.0"))
		float LeverLimitPositive;

	// The max angle of the lever in the negative direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent", meta = (ClampMin = "0.0", ClampMax = "179.8", UIMin = "0.0", UIMax = "180.0"))
		float LeverLimitNegative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		EVRInteractibleLeverReturnType LeverReturnTypeWhenReleased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		bool bSendLeverEventsDuringLerp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		float LeverReturnSpeed;

	// Number of frames to average momentum across for the release momentum (avoids quick waggles)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent|Momentum Settings", meta = (ClampMin = "0", ClampMax = "12", UIMin = "0", UIMax = "12"))
		int FramesToAverage;

	// Units in degrees per second to slow a momentum lerp down
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent|Momentum Settings", meta = (ClampMin = "0.0", ClampMax = "180", UIMin = "0.0", UIMax = "180.0"))
		float LeverMomentumFriction;

	// % of elasticity on reaching the end 0 - 1.0 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent|Momentum Settings", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float LeverRestitution;

	// Maximum momentum of the lever in degrees per second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent|Momentum Settings", meta = (ClampMin = "0.0", UIMin = "0.0"))
		float MaxLeverMomentum;

	UPROPERTY(BlueprintReadOnly, Category = "VRLeverComponent")
		bool bIsLerping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		int GripPriority;

	// Full precision current angle
	float FullCurrentAngle;

	float LastDeltaAngle;

	// Now replicating this so that it works correctly over the network
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_InitialRelativeTransform, Category = "VRLeverComponent")
	FTransform_NetQuantize InitialRelativeTransform;

	UFUNCTION()
	virtual void OnRep_InitialRelativeTransform()
	{
		CalculateCurrentAngle(InitialRelativeTransform);
	}

	FVector InitialInteractorLocation;
	FVector InitialInteractorDropLocation;
	float InitialGripRot;
	float RotAtGrab;
	FQuat qRotAtGrab;
	bool bLeverState;
	bool bIsInFirstTick;

	// For momentum retention
	float MomentumAtDrop;
	float LastLeverAngle;

	float CalcAngle(EVRInteractibleLeverAxis AxisToCalc, FVector CurInteractorLocation)
	{
		float ReturnAxis = 0.0f;

		switch (AxisToCalc)
		{
		case EVRInteractibleLeverAxis::Axis_X:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.Z)) - InitialGripRot;
		}break;
		case EVRInteractibleLeverAxis::Axis_Y:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Z, CurInteractorLocation.X)) - InitialGripRot;
		}break;
		case EVRInteractibleLeverAxis::Axis_Z:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.X)) - InitialGripRot;
		}break;
		default:
		{}break;
		}

		ReturnAxis = FMath::ClampAngle(FRotator::NormalizeAxis(RotAtGrab + ReturnAxis), -LeverLimitNegative, LeverLimitPositive);

		// Ignore rotations that would flip the angle of the lever to the other side, with a 90 degree allowance
		if (!bIsInFirstTick && ((LeverLimitPositive > 0.0f && LastDeltaAngle >= LeverLimitPositive) || (LeverLimitNegative > 0.0f && LastDeltaAngle <= -LeverLimitNegative)) && FMath::Sign(LastDeltaAngle) != FMath::Sign(ReturnAxis))
		{
			ReturnAxis = LastDeltaAngle;
		}
	
		bIsInFirstTick = false;
		return ReturnAxis;
	}

	/*float CalcAngleNumber(EVRInteractibleLeverAxis AxisToCalc, FTransform & CurrentRelativeTransform)
	{
		FQuat RotTransform = FQuat::Identity;

		if (AxisToCalc == EVRInteractibleLeverAxis::Axis_X)
			RotTransform = FRotator(FRotator(0.0, -90.0, 0.0)).Quaternion(); // Correct for roll and DotProduct

		FQuat newInitRot = (InitialRelativeTransform.GetRotation() * RotTransform);

		FVector v1 = (CurrentRelativeTransform.GetRotation() * RotTransform).Vector();
		FVector v2 = (newInitRot).Vector();
		v1.Normalize();
		v2.Normalize();

		FVector CrossP = FVector::CrossProduct(v1, v2);

		float angle = FMath::RadiansToDegrees(FMath::Atan2(CrossP.Size(), FVector::DotProduct(v1, v2)));
		angle *= FMath::Sign(FVector::DotProduct(CrossP, newInitRot.GetRightVector()));

		return angle;
	}*/

	void LerpAxis(float CurrentAngle, float DeltaTime)
	{
		float TargetAngle = 0.0f;
		float FinalReturnSpeed = LeverReturnSpeed;

		switch (LeverReturnTypeWhenReleased)
		{
		case EVRInteractibleLeverReturnType::LerpToMax:
		{
			if (CurrentAngle >= 0)
				TargetAngle = FMath::RoundToFloat(LeverLimitPositive);
			else
				TargetAngle = -FMath::RoundToFloat(LeverLimitNegative);
		}break;
		case EVRInteractibleLeverReturnType::LerpToMaxIfOverThreshold:
		{
			if ((!FMath::IsNearlyZero(LeverLimitPositive) && CurrentAngle >= (LeverLimitPositive * LeverTogglePercentage)))
				TargetAngle = FMath::RoundToFloat(LeverLimitPositive);
			else if ((!FMath::IsNearlyZero(LeverLimitNegative) && CurrentAngle <= -(LeverLimitNegative * LeverTogglePercentage)))
				TargetAngle = -FMath::RoundToFloat(LeverLimitNegative);
		}break;
		case EVRInteractibleLeverReturnType::RetainMomentum:
		{
			if (FMath::IsNearlyZero(MomentumAtDrop * DeltaTime, 0.1f))
			{
				MomentumAtDrop = 0.0f;
				this->SetComponentTickEnabled(false);
				bIsLerping = false;
				bReplicateMovement = true;
				return;
			}
			else
			{
				MomentumAtDrop = FMath::FInterpTo(MomentumAtDrop, 0.0f, DeltaTime, LeverMomentumFriction);

				FinalReturnSpeed = FMath::Abs(MomentumAtDrop);

				if (MomentumAtDrop >= 0.0f)
					TargetAngle = FMath::RoundToFloat(LeverLimitPositive);
				else
					TargetAngle = -FMath::RoundToFloat(LeverLimitNegative);
			}

		}break;
		case EVRInteractibleLeverReturnType::ReturnToZero:
		default:
		{}break;
		}

		//float LerpedVal = FMath::FixedTurn(CurrentAngle, TargetAngle, FinalReturnSpeed * DeltaTime);
		float LerpedVal = FMath::FInterpConstantTo(CurrentAngle, TargetAngle, DeltaTime, FinalReturnSpeed);

		if (FMath::IsNearlyEqual(LerpedVal, TargetAngle))
		{
			if (LeverRestitution > 0.0f)
			{
				MomentumAtDrop = -(MomentumAtDrop * LeverRestitution);
				this->SetRelativeRotation((FTransform(SetAxisValue(TargetAngle, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
			}
			else
			{
				this->SetComponentTickEnabled(false);
				bIsLerping = false;
				bReplicateMovement = true;
				this->SetRelativeRotation((FTransform(SetAxisValue(TargetAngle, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
			}
		}
		else
		{
			this->SetRelativeRotation((FTransform(SetAxisValue(LerpedVal, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
		}
	}

	void CalculateCurrentAngle(FTransform & CurrentRelativeTransform)
	{
		float Angle;

		switch (LeverRotationAxis)
		{
		case EVRInteractibleLeverAxis::Axis_XY:
		case EVRInteractibleLeverAxis::Axis_XZ:
		{
			// Manually calculating the angle here because RotationBetween() from FQuat uses Yaw/Pitch so roll would be incorrect
			FVector qAxis;
			float qAngle;

			(CurrentRelativeTransform * InitialRelativeTransform.Inverse()).GetRotation().ToAxisAndAngle(qAxis, qAngle);

			FullCurrentAngle = FMath::RadiansToDegrees(qAngle);
			CurrentLeverAngle = FMath::RoundToFloat(FullCurrentAngle);

			if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY)
				qAxis.Z = 0.0f; // Doing 2D axis values

			CurrentLeverForwardVector = -qAxis;

		}break;

		case EVRInteractibleLeverAxis::Axis_X:
		{
			Angle = GetAxisValue((CurrentRelativeTransform * InitialRelativeTransform.Inverse()).Rotator().GetNormalized());

			FullCurrentAngle = Angle;
			CurrentLeverAngle = FMath::RoundToFloat(FullCurrentAngle);
			CurrentLeverForwardVector = FVector(FMath::Sign(Angle), 0.0f, 0.0f);
		}break;
		case EVRInteractibleLeverAxis::Axis_Y:
		{
			Angle = GetAxisValue((CurrentRelativeTransform * InitialRelativeTransform.Inverse()).Rotator().GetNormalized());
			
			FullCurrentAngle = Angle;
			CurrentLeverAngle = FMath::RoundToFloat(FullCurrentAngle);
			CurrentLeverForwardVector = FVector(0.0f, FMath::Sign(Angle), 0.0f);
		}break;
		case EVRInteractibleLeverAxis::Axis_Z:
		{
			Angle = GetAxisValue((CurrentRelativeTransform * InitialRelativeTransform.Inverse()).Rotator().GetNormalized());

			FullCurrentAngle = Angle;
			CurrentLeverAngle = FMath::RoundToFloat(FullCurrentAngle);
			CurrentLeverForwardVector = FVector(0.0f, 0.0f, FMath::Sign(Angle));
		}break;

		default:
		{}break;
		}
	}

	FTransform GetCurrentParentTransform()
	{
		if (ParentComponent.IsValid())
		{
			// during grip there is no parent so we do this, might as well do it anyway for lerping as well
			return ParentComponent->GetComponentTransform();
		}
		else
		{
			return FTransform::Identity;
		}
	}

	// ------------------------------------------------
	// Gameplay tag interface
	// ------------------------------------------------

	/** Overridden to return requirements tags */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = GameplayTags;
	}

	/** Tags that are set on this object */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;

	// End Gameplay Tag Interface

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;
	

	// Requires bReplicates to be true for the component
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface")
		bool bRepGameplayTags;
		
	// Overrides the default of : true and allows for controlling it like in an actor, should be default of off normally with grippable components
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface|Replication")
		bool bReplicateMovement;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		EGripMovementReplicationSettings MovementReplicationSetting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float BreakDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float Stiffness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float Damping;

	UPROPERTY(BlueprintReadWrite, Category = "VRGripInterface")
		bool bDenyGripping;

	UPROPERTY(BlueprintReadOnly, Category = "VRGripInterface")
		bool bIsHeld; // Set on grip notify, not net serializing

	UPROPERTY(BlueprintReadOnly, Category = "VRGripInterface")
		UGripMotionControllerComponent * HoldingController; // Set on grip notify, not net serializing

	TWeakObjectPtr<USceneComponent> ParentComponent;

	// Should be called after the lever is moved post begin play
	UFUNCTION(BlueprintCallable, Category = "VRLeverComponent")
	void ResetInitialLeverLocation()
	{
		// Get our initial relative transform to our parent (or not if un-parented).
		InitialRelativeTransform = this->GetRelativeTransform();
		CalculateCurrentAngle(InitialRelativeTransform);
	}

	// ReCalculates the current angle, sets it on the back end, and returns it
	UFUNCTION(BlueprintCallable, Category = "VRLeverComponent")
	float ReCalculateCurrentAngle()
	{
		FTransform CurRelativeTransform = this->GetComponentTransform().GetRelativeTransform(GetCurrentParentTransform());
		CalculateCurrentAngle(CurRelativeTransform);
		return CurrentLeverAngle;
	}

	virtual void OnUnregister() override;;

#if WITH_PHYSX
	physx::PxD6Joint* HandleData;
	//int32 SceneIndex;
#endif

	bool DestroyConstraint()
	{
	#if WITH_PHYSX
		if (HandleData)
		{
			// use correct scene
			PxScene* PScene = HandleData->getScene();//GetPhysXSceneFromIndex(SceneIndex);
			if (PScene)
			{
				PScene->lockWrite();
				//SCOPED_SCENE_WRITE_LOCK(PScene);
				// Destroy joint.
				HandleData->release();
				PScene->unlockWrite();
			}

			HandleData = NULL;
			return true;
		}
		else
		{
			return false;
		}
	#endif // WITH_PHYSX

		return true;
	}

	bool SetupConstraint()
	{
#if WITH_PHYSX
		
		if (HandleData)
			return true;

		// Get the PxRigidDynamic that we want to grab.
		FBodyInstance* rBodyInstance = this->GetBodyInstance(NAME_None);
		if (!rBodyInstance)
		{
			return false;
		}


		FTransform A2Transform = FTransform::Identity;//GetComponentTransform().Inverse();
		if (ParentComponent.IsValid())
		{
			UPrimitiveComponent * PrimComp = Cast<UPrimitiveComponent>(ParentComponent.Get());

			if (PrimComp)
				A2Transform = PrimComp->GetComponentTransform();
		}

		float rotationalOffset = (LeverLimitPositive - LeverLimitNegative) / 2;
		FRotator AngularRotationOffset = SetAxisValue(rotationalOffset, FRotator::ZeroRotator);
		FTransform RefFrame2 = FTransform(InitialRelativeTransform.GetRotation() * AngularRotationOffset.Quaternion(), A2Transform.InverseTransformPosition(GetComponentLocation()));
		
		// If we don't already have a handle - make one now.
		if (!HandleData)
		{
			FPhysicsCommand::ExecuteWrite(BodyInstance.ActorHandle, [&](const FPhysicsActorHandle& Actor)
				//ExecuteOnPxRigidDynamicReadWrite(rBodyInstance, [&](PxRigidDynamic* Actor)
			{
				if (PxRigidActor* PActor = FPhysicsInterface::GetPxRigidActor_AssumesLocked(Actor))
				{
					PxScene* Scene = PActor->getScene();
					PxD6Joint* NewJoint = NULL;
					PxRigidDynamic * ParentBody = NULL;

					if (ParentComponent.IsValid())
					{
						UPrimitiveComponent * PrimComp = Cast<UPrimitiveComponent>(ParentComponent.Get());

						if (PrimComp && PrimComp->BodyInstance.IsValidBodyInstance())
						{
							ParentBody = FPhysicsInterface::GetPxRigidDynamic_AssumesLocked(PrimComp->BodyInstance.ActorHandle);
							//ParentBody = PrimComp->BodyInstance.GetPxRigidDynamic_AssumesLocked();
						}
					}

					NewJoint = PxD6JointCreate(Scene->getPhysics(), ParentBody, U2PTransform(RefFrame2), PActor, PxTransform(PxIdentity));

					if (!NewJoint)
					{
						HandleData = NULL;
					}
					else
					{
						// No constraint instance
						NewJoint->userData = NULL; // don't need
						HandleData = NewJoint;

						// Remember the scene index that the handle joint/actor are in.
						FPhysScene* RBScene = FPhysxUserData::Get<FPhysScene>(Scene->userData);
						const uint32 SceneType = rBodyInstance->UseAsyncScene(RBScene) ? PST_Async : PST_Sync;
						//SceneIndex = RBScene->PhysXSceneIndex[SceneType];

						// Pretty Much Unbreakable
						NewJoint->setBreakForce(PX_MAX_REAL, PX_MAX_REAL);
						//	NewJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);

						//	NewJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);

						PxConstraintFlags Flags = NewJoint->getConstraintFlags();

						// False flags
						//Flags |= PxConstraintFlag::ePROJECTION;
						Flags |= PxConstraintFlag::eCOLLISION_ENABLED;

						// True flags
						Flags &= ~PxConstraintFlag::ePROJECTION;

						NewJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
						NewJoint->setProjectionAngularTolerance(FMath::DegreesToRadians(0.1f));
						NewJoint->setProjectionLinearTolerance(0.1f);
						NewJoint->setConstraintFlags(Flags);

						// Setting up the joint
						NewJoint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
						NewJoint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
						NewJoint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

						NewJoint->setMotion(PxD6Axis::eTWIST, LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X || LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
						NewJoint->setMotion(PxD6Axis::eSWING1, LeverRotationAxis == EVRInteractibleLeverAxis::Axis_Y || LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
						NewJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);

						const float CorrectedLeverLimit = (LeverLimitPositive + LeverLimitNegative) / 2;
						const float LeverLimitRad = CorrectedLeverLimit * (PI / 180.0f);
						//PxReal LimitContactDistance = FMath::DegreesToRadians(FMath::Max(1.f, ProfileInstance.ConeLimit.ContactDistance));

						//The limit values need to be clamped so it will be valid in PhysX
						PxReal ZLimitAngle = FMath::ClampAngle(CorrectedLeverLimit, KINDA_SMALL_NUMBER, 179.9999f) * (PI / 180.0f);
						PxReal YLimitAngle = FMath::ClampAngle(CorrectedLeverLimit, KINDA_SMALL_NUMBER, 179.9999f) * (PI / 180.0f);
						//PxReal LimitContactDistance = FMath::DegreesToRadians(FMath::Max(1.f, ProfileInstance.ConeLimit.ContactDistance * FMath::Min(InSwing1LimitScale, InSwing2LimitScale)));

						NewJoint->setSwingLimit(PxJointLimitCone(YLimitAngle, ZLimitAngle));
						NewJoint->setTwistLimit(PxJointAngularLimitPair(-LeverLimitRad, LeverLimitRad));

						return true;
					}
				}
				return false;
			});
		}
		
#else
		return false;
#endif // WITH_PHYSX

		return false;
	}


	// Grip interface setup

	// Set up as deny instead of allow so that default allows for gripping
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface", meta = (DisplayName = "IsDenyingGrips"))
		bool DenyGripping();

	// How an interfaced object behaves when teleporting
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripInterfaceTeleportBehavior TeleportBehavior();

	// Should this object simulate on drop
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		bool SimulateOnDrop();

	/*// Grip type to use when gripping a slot
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripCollisionType SlotGripType();

	// Grip type to use when not gripping a slot
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripCollisionType FreeGripType();
		*/

	// Secondary grip type
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		ESecondaryGripType SecondaryGripType();

		// Grip type to use
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripCollisionType GetPrimaryGripType(bool bIsSlot);

	// Define which movement repliation setting to use
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripMovementReplicationSettings GripMovementReplicationType();

	// Define the late update setting
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripLateUpdateSettings GripLateUpdateSetting();

	/*// What grip stiffness to use if using a physics constraint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		float GripStiffness();

	// What grip damping to use if using a physics constraint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		float GripDamping();
		*/
		// What grip stiffness and damping to use if using a physics constraint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void GetGripStiffnessAndDamping(float &GripStiffnessOut, float &GripDampingOut);

	// Get the advanced physics settings for this grip
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		FBPAdvGripSettings AdvancedGripSettings();

	// What distance to break a grip at (only relevent with physics enabled grips
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		float GripBreakDistance();

	/*// Get closest secondary slot in range
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void ClosestSecondarySlotInRange(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController = nullptr, FName OverridePrefix = NAME_None);

	// Get closest primary slot in range
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void ClosestPrimarySlotInRange(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController = nullptr, FName OverridePrefix = NAME_None);
		*/

	// Get grip primary slot in range
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void ClosestGripSlotInRange(FVector WorldLocation, bool bSecondarySlot, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController = nullptr, FName OverridePrefix = NAME_None);




	// Check if the object is an interactable
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		//bool IsInteractible();

	// Returns if the object is held and if so, which pawn is holding it
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void IsHeld(UGripMotionControllerComponent *& CurHoldingController, bool & bCurIsHeld);

	// Sets is held, used by the plugin
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void SetHeld(UGripMotionControllerComponent * NewHoldingController, bool bNewIsHeld);

	// Returns if the object is socketed currently
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		bool RequestsSocketing(USceneComponent *& ParentToSocketTo, FName & OptionalSocketName, FTransform_NetQuantize & RelativeTransform);

	// Get interactable settings
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		//FBPInteractionSettings GetInteractionSettings();

	// Get grip scripts
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		bool GetGripScripts(TArray<UVRGripScriptBase*> & ArrayReference);

	// Events //

	// Event triggered each tick on the interfaced object when gripped, can be used for custom movement or grip based logic
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void TickGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation, float DeltaTime);

	// Event triggered on the interfaced object when gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when grip is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnGripRelease(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed = false);

	// Event triggered on the interfaced object when child component is gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnChildGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when child component is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnChildGripRelease(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed = false);

	// Event triggered on the interfaced object when secondary gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnSecondaryGrip(USceneComponent * SecondaryGripComponent, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when secondary grip is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnSecondaryGripRelease(USceneComponent * ReleasingSecondaryGripComponent, const FBPActorGripInformation & GripInformation);

	// Interaction Functions

	// Call to use an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnUsed();

	// Call to stop using an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnEndUsed();

	// Call to use an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnSecondaryUsed();

	// Call to stop using an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnEndSecondaryUsed();

	// Call to send an action event to the object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnInput(FKey Key, EInputEvent KeyEvent);

	protected:

		inline float GetAxisValue(FRotator CheckRotation)
		{
			switch (LeverRotationAxis)
			{
			case EVRInteractibleLeverAxis::Axis_X:
				return CheckRotation.Roll; break;
			case EVRInteractibleLeverAxis::Axis_Y:
				return CheckRotation.Pitch; break;
			case EVRInteractibleLeverAxis::Axis_Z:
				return CheckRotation.Yaw; break;
			default:return 0.0f; break;
			}
		}

		inline FRotator SetAxisValue(float SetValue)
		{
			FRotator vec = FRotator::ZeroRotator;

			switch (LeverRotationAxis)
			{
			case EVRInteractibleLeverAxis::Axis_X:
				vec.Roll = SetValue; break;
			case EVRInteractibleLeverAxis::Axis_Y:
				vec.Pitch = SetValue; break;
			case EVRInteractibleLeverAxis::Axis_Z:
				vec.Yaw = SetValue; break;
			default:break;
			}

			return vec;
		}

		inline FRotator SetAxisValue(float SetValue, FRotator Var)
		{
			FRotator vec = Var;
			switch (LeverRotationAxis)
			{
			case EVRInteractibleLeverAxis::Axis_X:
				vec.Roll = SetValue; break;
			case EVRInteractibleLeverAxis::Axis_Y:
				vec.Pitch = SetValue; break;
			case EVRInteractibleLeverAxis::Axis_Z:
				vec.Yaw = SetValue; break;
			default:break;
			}

			return vec;
		}

};