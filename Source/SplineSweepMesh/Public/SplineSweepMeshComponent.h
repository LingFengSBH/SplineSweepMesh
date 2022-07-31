// Copyright 2022 Sun Boheng.All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInterface.h"
#include "SplineSweepMeshComponent.generated.h"


class USplineComponent;

struct TrianglePoints
{
	FVector pointA = FVector();
	FVector pointB = FVector();
	FVector pointC = FVector();

public:
	FORCEINLINE TrianglePoints(FVector A, FVector B, FVector C);

};

struct QuadPoints
{
	FVector pointA = FVector();
	FVector pointB = FVector();
	FVector pointC = FVector();
	FVector pointD = FVector();

	FVector2D UVa = FVector2D();
	FVector2D UVb = FVector2D();
	FVector2D UVc = FVector2D();
	FVector2D UVd = FVector2D();

public:
	FORCEINLINE QuadPoints(
		FVector A, FVector B, FVector C, FVector D, FVector2D UA,
		FVector2D UB, FVector2D UC, FVector2D UD);
};

UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable)
class SPLINESWEEPMESH_API USplineSweepMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	/**
	 *	Create spline sweep mesh with two splines
	 *	@param	SplineToSweep		    A spline component reference which is used to sweep along path
	 *	@param	SplineAsPath		    A spline component reference which is used as path
	 *	@param	NumberOfSegments		How many segments should be created along path
	 *	@param	RateOfProgress		    To make grow animation.Rate of grow progress along path
	 *	@param	SmoothNormal		    Whether should use smoothed normal for side surface,vertex would be shared if use smoothed normal
	 *	@param	CreateCollision	        Whether collision should be created for this section. This adds significant cost.
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineSweep")
		void CreateSweepMesh(USplineComponent* SplineToSweep, USplineComponent* SplineAsPath, int NumberOfSegments, float RateOfProgress, bool SmoothNormal, bool CreateCollision);
	/**
	 *	Updates spline sweep mesh. This is faster than CreateCreateSweepMesh, but does not let you change topology. Collision info is also updated.
	 *	@param	Path		            A spline component reference which is used as path
	 * 	@param	RateOfProgress		    To make grow animation.Rate of grow progress along path
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineSweep")
		void UpdatePathSpline(USplineComponent* Path ,float RateOfProgress);


protected:
	//Store if use smooth normal
	bool bUseSmoothNormal;
	//If spline of path is closed loop,two covers will be created
	bool bHaveCover;
	//Store the number of segments
	int NumSegments;
	//Store points' positions of spline to sweep
	TArray<FVector> SweepPoints;
	//Store points' normals of spline to sweep
	TArray<FVector> SweepPointNormals;
	//Store spline area triangles of spline to sweep
	TArray<TrianglePoints> CoverTriangles;

	//Create mesh sections
	//Create flank surface along path spline.Section 0
	void CreateSideQuads(USplineComponent* PathSpline, USplineComponent* SweepSpline, int SegmentsNumber, float Rate, bool bSmooth, bool CreateCollision);
	//Create two covers surface if path spline is not closed loop.Section 1
	void CreateCoverTriangles(USplineComponent* PathSpline, USplineComponent* SweepSpline, float Rate, bool CreateCollision);

	//Update mesh section
	//Update flank surface along path spline.Section 0
	void UpdateSideQuads(USplineComponent* path, float Rate);
	//Update two covers surface position if have covers.Section 1
	void UpdateCoverTriangles(USplineComponent* path, float Rate);

	//Sweep points' position and normal along path spline.Used to create side surface
	void SweepPointsAlongSpline(USplineComponent* path, TArray<FVector>& PointsToSweep, TArray<FVector>& NormalsToSweep, int SegmentsNumber, float Rate, TArray<FVector>& OutPoints, TArray<FVector>& OutNormal, TArray<FVector2D>& OutUVs);
	//Use cross to find triangle in the spline area.Used to create cover
	TrianglePoints FindAndRemoveFirstTriangleInSplineArea(TArray<FVector>& points);
	//Convert a spline area into triangles to create cover
	TArray<TrianglePoints> ConvertSplineIntoTriangle(TArray<FVector>& points);
	//Get local positions of all points of spline 
	TArray<FVector> GetSplinePointsLocation(USplineComponent* spline);
	//Get local normals of all points of spline 
	TArray<FVector> GetSplinePointsNormal(USplineComponent* spline);
	//Get transform matrix along spline at distance
	FMatrix GetMatrixInSplineDistance(USplineComponent* spline, float distance)const;
};
