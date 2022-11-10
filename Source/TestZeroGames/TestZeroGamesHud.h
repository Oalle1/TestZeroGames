// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "TestZeroGamesHud.generated.h"


UCLASS(config = Game)
class ATestZeroGamesHud : public AHUD
{
	GENERATED_BODY()

public:
	ATestZeroGamesHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
