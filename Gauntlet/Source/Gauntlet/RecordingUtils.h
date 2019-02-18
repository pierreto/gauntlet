// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/SkeletalMeshSocket.h"

#include "RecordingUtils.generated.h"

/**
 * 
 */
UCLASS()
class GAUNTLET_API URecordingUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Category = "Recording")
	static void SetSkeletalMeshSocketTransform(USkeletalMeshSocket* socket, FTransform transform);
};
