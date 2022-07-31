// Copyright 2022 Sun Boheng.All Rights Reserved.

#include "SplineSweepMeshActor.h"
#include "UObject/ConstructorHelpers.h"


// Sets default values
ASplineSweepMeshActor::ASplineSweepMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SweepMeshComponent = CreateDefaultSubobject<USplineSweepMeshComponent>(TEXT("SweepMeshComponent"));

	RootComponent = SweepMeshComponent;

	if (SideMaterial == nullptr)
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FindMaterial(
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (FindMaterial.Succeeded())
		{
			SideMaterial = FindMaterial.Object;
		}
	}

	if (CoverMaterial == nullptr)
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FindMaterial(
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (FindMaterial.Succeeded())
		{
			CoverMaterial = FindMaterial.Object;
		}
	}

		SweepMeshComponent->SetMaterial(0, SideMaterial);
		SweepMeshComponent->SetMaterial(1, CoverMaterial);


	SplineToSweep = CreateDefaultSubobject<USplineComponent>(TEXT("SplineToSweep"));
	SplineAsPath = CreateDefaultSubobject<USplineComponent>(TEXT("SplineAsPath"));

	SplineAsPath->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	SplineToSweep->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SplineAsPath->SetRelativeLocation(FVector(0, 0, 0));
	SplineToSweep->SetRelativeLocation(FVector(0, 100, 0));
	SplineToSweep->ClearSplinePoints();
	SplineToSweep->SetClosedLoop(true);

	SplineToSweep->AddSplineLocalPoint(FVector(0, -20, -20));
	SplineToSweep->AddSplineLocalPoint(FVector(0, -20, 20));
	SplineToSweep->AddSplineLocalPoint(FVector(0, 20, 20));
	SplineToSweep->AddSplineLocalPoint(FVector(0, 20, -20));

	
}

void ASplineSweepMeshActor::OnConstruction(const FTransform& Transform)
{
	SplineToSweep->SetClosedLoop(true);
	for (int i = 0; i < SplineToSweep->GetNumberOfSplinePoints(); i++)
	{
		SplineToSweep->SetSplinePointType(i, ESplinePointType::Linear, false);
	}
	CreateSweepMesh();
	SweepMeshComponent->SetMaterial(0, SideMaterial);
	SweepMeshComponent->SetMaterial(1, CoverMaterial);
}

void ASplineSweepMeshActor::CreateSweepMesh()
{
	SweepMeshComponent->CreateSweepMesh(SplineToSweep, SplineAsPath, NumSegments,RateOfProgress, bUseSmoothNormal, bCreateCollision);
}

void ASplineSweepMeshActor::UpdatePathSpline(float Rate)
{
	RateOfProgress = Rate;
	SweepMeshComponent->UpdatePathSpline(SplineAsPath,Rate);
}

// Called when the game starts or when spawned
void ASplineSweepMeshActor::BeginPlay()
{
	Super::BeginPlay();
	CreateSweepMesh();
}

// Called every frame
void ASplineSweepMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


