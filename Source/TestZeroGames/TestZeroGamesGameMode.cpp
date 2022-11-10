// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestZeroGamesGameMode.h"
#include "TestZeroGamesPawn.h"
#include "TestZeroGamesHud.h"

ATestZeroGamesGameMode::ATestZeroGamesGameMode()
{
	DefaultPawnClass = ATestZeroGamesPawn::StaticClass();
	//HUDClass = ATestZeroGamesHud::StaticClass();
}
void ATestZeroGamesGameMode::BeginPlay()
{
	Super::BeginPlay();
	ChangeMenuWidget(StartingWidgetClass);
}
void ATestZeroGamesGameMode::ChangeMenuWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
    if (CurrentWidget != nullptr)
    {
        CurrentWidget->RemoveFromViewport();
        CurrentWidget = nullptr;
    }
    if (NewWidgetClass != nullptr)
    {
        CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), NewWidgetClass);
        if (CurrentWidget != nullptr)
        {
            CurrentWidget->AddToViewport();
        }
    }
}