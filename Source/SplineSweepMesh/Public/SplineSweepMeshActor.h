// Copyright 2022 Sun Boheng.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "SplineSweepMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "SplineSweepMeshActor.generated.h"

UCLASS()
class SPLINESWEEPMESH_API ASplineSweepMeshActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASplineSweepMeshActor();

	//A spline component reference which is used to sweep along path
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "SplineSweep")
		USplineComponent* SplineToSweep = nullptr;
	//A spline component reference which is used as path
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "SplineSweep")
		USplineComponent* SplineAsPath = nullptr;

	//Materials
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineSweep")
		UMaterialInterface* CoverMaterial = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineSweep")
		UMaterialInterface* SideMaterial = nullptr;

	//Whether should use smoothed normal for side surface,verticles would be shared if use smoothed normal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineSweep")
		bool bUseSmoothNormal;
	//Whether collision should be created for this section.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineSweep")
		bool bCreateCollision;
	//How many degments should be created along path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineSweep")
		int NumSegments = 10;
	//To make grow animation.Rate of grow progress along path.Should be in[0,1]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineSweep")
		float RateOfProgress = 1;

	//Spline sweep mesh component
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Default)
		USplineSweepMeshComponent* SweepMeshComponent;

	//Call SweepMeshComponent->CreateSweepMesh();
	UFUNCTION(BlueprintCallable, Category = "SplineSweep")
		void CreateSweepMesh();
	//Call 	SweepMeshComponent->UpdatePathSpline();
	UFUNCTION(BlueprintCallable, Category = "SplineSweep")
		void UpdatePathSpline(float Progress);



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
};
