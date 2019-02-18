// Fill out your copyright notice in the Description page of Project Settings.

#include "RecordingUtils.h"
#include "Engine/GameEngine.h"
#include "Engine/SkeletalMesh.h"

void URecordingUtils::SetSkeletalMeshSocketTransform(USkeletalMeshSocket* socket, FTransform transform) {
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(10, 15.0f, FColor::Yellow, TEXT("Some debug message!"));
	

}