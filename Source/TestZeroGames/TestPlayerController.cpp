// Fill out your copyright notice in the Description page of Project Settings.


#include "TestPlayerController.h"

void ATestPlayerController::BeginPlay()
{
    Super::BeginPlay();
    SetInputMode(FInputModeGameAndUI());
}