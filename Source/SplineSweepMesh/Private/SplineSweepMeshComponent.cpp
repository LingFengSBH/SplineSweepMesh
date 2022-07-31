// Copyright 2022 Sun Boheng.All Rights Reserved.

#include "SplineSweepMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetProceduralMeshLibrary.h"

TrianglePoints::TrianglePoints(FVector A, FVector B, FVector C)
{

	pointA = A;
	pointB = B;
	pointC = C;
}

QuadPoints::QuadPoints(FVector A, FVector B, FVector C, FVector D, FVector2D UA, FVector2D UB, FVector2D UC, FVector2D UD)
{
	pointA = A;
	pointB = B;
	pointC = C;
	pointD = D;


	UVa = UA;
	UVb = UB;
	UVc = UC;
	UVd = UD;
}



void USplineSweepMeshComponent::CreateSweepMesh(USplineComponent* SweepSpline, USplineComponent* PathSpline, int segments, float Rate, bool SmoothNormal, bool CreateCollision)
{
	//Clear procedural mesh sections
	ClearAllMeshSections();
	//Is valid
	if (PathSpline && SweepSpline)
	{
		//Store points' info which will be used to sweep along path
		SweepPoints = GetSplinePointsLocation(SweepSpline);
		SweepPointNormals = GetSplinePointsNormal(SweepSpline);

		bUseSmoothNormal = SmoothNormal;
		NumSegments = segments;
		CreateSideQuads(PathSpline, SweepSpline, segments,Rate, SmoothNormal, CreateCollision);

		//If is closed loop,create covers 
		if (!PathSpline->IsClosedLoop())
		{
			CreateCoverTriangles(PathSpline, SweepSpline,Rate, CreateCollision);
			bHaveCover = true;
		}
		else
		{
			bHaveCover = false;
		}
	}
}

void USplineSweepMeshComponent::UpdatePathSpline(USplineComponent* path, float Rate)
{
	UpdateSideQuads(path,Rate);
	if (bHaveCover)
	{
		UpdateCoverTriangles(path,Rate);
	}
}

void USplineSweepMeshComponent::CreateSideQuads(USplineComponent* PathSpline, USplineComponent* SweepSpline, int SegmentsNumber, float Rate, bool bSmooth, bool CreateCollision)
{
	TArray<FVector> SidePoints;
	TArray<FVector> PointNormals;
	TArray<FVector2D> SideUVs;

	//Use  SweepPoints and SweepPointNormals to sweep along path to create side points 
	SweepPointsAlongSpline(PathSpline, SweepPoints, SweepPointNormals, SegmentsNumber, Rate,SidePoints, PointNormals, SideUVs);

	//Parameters used to create procedural mesh 
	TArray<FVector> vertices;
	TArray<int> indices;
	TArray<FVector> normals;
	TArray<FVector2D> UVs;
	TArray<FColor> color;
	TArray<FProcMeshTangent> tangent;

	//Branch if use smoothed normal
	if (bSmooth)
	{
		vertices = SidePoints;
		normals = PointNormals;
		UVs = SideUVs;

		//Create triangles
		for (int i = 0; i < SegmentsNumber; i++)
		{
			for (int j = 0; j < SweepPoints.Num(); j++)
			{
				int p1 = i * SweepPoints.Num();
				int p2 = (i + 1) * SweepPoints.Num();
				int n = (j + 1) == SweepPoints.Num() ? 0 : (j + 1);

				indices.Add(p1 + j);
				indices.Add(p1 + n);
				indices.Add(p2 + j);
				indices.Add(p1 + n);
				indices.Add(p2 + n);
				indices.Add(p2 + j);
			}
		}

	}
	else
	{
		//Convert side points into quad
		TArray<QuadPoints> SideQuad;
		for (int i = 0; i < SegmentsNumber; i++)
		{
			for (int j = 0; j < SweepPoints.Num(); j++)
			{
				int p1 = i * SweepPoints.Num();
				int p2 = (i + 1) * SweepPoints.Num();
				int n = (j + 1) == SweepPoints.Num() ? 0 : (j + 1);
				FVector2D UVc = (j + 1) == SweepPoints.Num() ? FVector2D(1, SideUVs[p1 + n].Y) : SideUVs[p1 + n];
				FVector2D UVd = (j + 1) == SweepPoints.Num() ? FVector2D(1, SideUVs[p2 + n].Y) : SideUVs[p2 + n];

				QuadPoints quad = QuadPoints(SidePoints[p1 + j], SidePoints[p2 + j], SidePoints[p1 + n], SidePoints[p2 + n],
					SideUVs[p1 + j], SideUVs[p2 + j], UVc, UVd);
				SideQuad.Add(quad);

			}
		}
		for (int i = 0; i < SideQuad.Num(); i++)
		{
			QuadPoints quad = SideQuad[i];

			vertices.Add(quad.pointA);
			vertices.Add(quad.pointB);
			vertices.Add(quad.pointC);
			vertices.Add(quad.pointD);

			UVs.Add(quad.UVa);
			UVs.Add(quad.UVb);
			UVs.Add(quad.UVc);
			UVs.Add(quad.UVd);

			indices.Add(i * 4);
			indices.Add(i * 4 + 2);
			indices.Add(i * 4 + 1);
			indices.Add(i * 4 + 2);
			indices.Add(i * 4 + 3);
			indices.Add(i * 4 + 1);

			//Calculate normal of triangles
			FVector n1 = UKismetMathLibrary::Cross_VectorVector(quad.pointB - quad.pointA, quad.pointC - quad.pointA).GetSafeNormal();
			FVector n2 = UKismetMathLibrary::Cross_VectorVector(quad.pointB - quad.pointC, quad.pointD - quad.pointC).GetSafeNormal();
			FVector n3 = (n1 + n2) / 2;

			normals.Add(n1);
			normals.Add(n3);
			normals.Add(n3);
			normals.Add(n2);
		}
	}
	CreateMeshSection(0, vertices, indices, normals, UVs, color, tangent, CreateCollision);

}

void USplineSweepMeshComponent::CreateCoverTriangles(USplineComponent* PathSpline, USplineComponent* SweepSpline, float Rate, bool CreateCollision)
{
	//Function "ConvertSplineIntoTriangle" would remove points in array to create triangles,copy SweepPoints to prevent it from being removed
	TArray<FVector> TemPoints = SweepPoints;
	CoverTriangles = ConvertSplineIntoTriangle(TemPoints);

	//Parameters used to create procedural mesh 
	TArray<FVector> vertices;
	TArray<int> indices;
	TArray<FVector> normals;
	TArray<FVector2D> UVs;
	TArray<FColor> color;
	TArray<FProcMeshTangent> tangents;

	//Create covers 
	for (int i = 0; i < CoverTriangles.Num(); i++)
	{
		//Transform matrix at start and end of path
		FMatrix M0 = GetMatrixInSplineDistance(PathSpline, 0);
		FMatrix M1 = GetMatrixInSplineDistance(PathSpline, PathSpline->GetSplineLength()*Rate);

		//Transform location of points into start and end of path 
		FVector A0 = UKismetMathLibrary::Matrix_TransformPosition(M0, CoverTriangles[i].pointA);
		FVector B0 = UKismetMathLibrary::Matrix_TransformPosition(M0, CoverTriangles[i].pointB);
		FVector C0 = UKismetMathLibrary::Matrix_TransformPosition(M0, CoverTriangles[i].pointC);
		FVector A1 = UKismetMathLibrary::Matrix_TransformPosition(M1, CoverTriangles[i].pointA);
		FVector B1 = UKismetMathLibrary::Matrix_TransformPosition(M1, CoverTriangles[i].pointB);
		FVector C1 = UKismetMathLibrary::Matrix_TransformPosition(M1, CoverTriangles[i].pointC);

		vertices.Add(A0);
		vertices.Add(C0);
		vertices.Add(B0);
		vertices.Add(A1);
		vertices.Add(B1);
		vertices.Add(C1);

		indices.Add(i * 6);
		indices.Add(i * 6 + 1);
		indices.Add(i * 6 + 2);
		indices.Add(i * 6 + 3);
		indices.Add(i * 6 + 4);
		indices.Add(i * 6 + 5);

		//Calculate triangle's normal
		FVector n0 = UKismetMathLibrary::Cross_VectorVector(B0 - A0, C0 - A0).GetSafeNormal();
		FVector n1 = UKismetMathLibrary::Cross_VectorVector(C1 - A1, B1 - A1).GetSafeNormal();

		normals.Add(n0);
		normals.Add(n0);
		normals.Add(n0);
		normals.Add(n1);
		normals.Add(n1);
		normals.Add(n1);
	}
	CreateMeshSection(1, vertices, indices, normals, UVs, color, tangents, CreateCollision);
}

void USplineSweepMeshComponent::UpdateSideQuads(USplineComponent* path,float Rate )
{
	//Parameters used to create procedural mesh 
	TArray<FVector> vertices;
	TArray<int> indices;
	TArray<FVector> normals;
	TArray<FVector2D> UVs;
	TArray<FColor> color;
	TArray<FProcMeshTangent> tangents;

	TArray<FVector> SidePoints;
	TArray<FVector> PointNormals;
	TArray<FVector2D> PointUVs;

	//Use  SweepPoints and SweepPointNormals to sweep along path to create side points 
	SweepPointsAlongSpline(path, SweepPoints, SweepPointNormals, NumSegments, Rate,SidePoints, PointNormals, PointUVs);


	//Branch when created,if user use smoothed normal or not
	if (bUseSmoothNormal)
	{
		vertices = SidePoints;
		normals = PointNormals;
		UVs = PointUVs;
	}
	else
	{
		//Convert side points into quad
		TArray<QuadPoints> SideQuad;
		for (int i = 0; i < NumSegments; i++)
		{
			for (int j = 0; j < SweepPoints.Num(); j++)
			{
				int p1 = i * SweepPoints.Num();
				int p2 = (i + 1) * SweepPoints.Num();
				int n = (j + 1) == SweepPoints.Num() ? 0 : (j + 1);
				FVector2D UVc = (j + 1) == SweepPoints.Num() ? FVector2D(1, PointUVs[p1 + n].Y) : PointUVs[p1 + n];
				FVector2D UVd = (j + 1) == SweepPoints.Num() ? FVector2D(1, PointUVs[p2 + n].Y) : PointUVs[p2 + n];

				QuadPoints quad = QuadPoints(SidePoints[p1 + j], SidePoints[p2 + j], SidePoints[p1 + n], SidePoints[p2 + n],
					PointUVs[p1 + j], PointUVs[p2 + j], UVc, UVd);
				SideQuad.Add(quad);

			}
		}
		for (int i = 0; i < SideQuad.Num(); i++)
		{
			QuadPoints quad = SideQuad[i];

			vertices.Add(quad.pointA);
			vertices.Add(quad.pointB);
			vertices.Add(quad.pointC);
			vertices.Add(quad.pointD);

			UVs.Add(quad.UVa);
			UVs.Add(quad.UVb);
			UVs.Add(quad.UVc);
			UVs.Add(quad.UVd);

			//Calculate normal of triangles
			FVector n1 = UKismetMathLibrary::Cross_VectorVector(quad.pointB - quad.pointA, quad.pointC - quad.pointA).GetSafeNormal();
			FVector n2 = UKismetMathLibrary::Cross_VectorVector(quad.pointB - quad.pointC, quad.pointD - quad.pointC).GetSafeNormal();
			FVector n3 = (n1 + n2) / 2;

			normals.Add(n1);
			normals.Add(n3);
			normals.Add(n3);
			normals.Add(n2);
		}
	}
	UpdateMeshSection(0, vertices, normals, UVs, color, tangents);
}

void USplineSweepMeshComponent::UpdateCoverTriangles(USplineComponent* path,float Rate )
{
	//Parameters used to create procedural mesh 
	TArray<FVector> vertices;
	TArray<int> indices;
	TArray<FVector> normals;
	TArray<FVector2D> UVs;
	TArray<FColor> color;
	TArray<FProcMeshTangent> tangents;

	for (int i = 0; i < CoverTriangles.Num(); i++)
	{
		//Transform matrix at start and end of path
		FMatrix M0 = GetMatrixInSplineDistance(path, 0);
		FMatrix M1 = GetMatrixInSplineDistance(path, path->GetSplineLength()*Rate);

		//Transform location of points into start and end of path 
		FVector A0 = UKismetMathLibrary::Matrix_TransformPosition(M0, CoverTriangles[i].pointA);
		FVector B0 = UKismetMathLibrary::Matrix_TransformPosition(M0, CoverTriangles[i].pointB);
		FVector C0 = UKismetMathLibrary::Matrix_TransformPosition(M0, CoverTriangles[i].pointC);
		FVector A1 = UKismetMathLibrary::Matrix_TransformPosition(M1, CoverTriangles[i].pointA);
		FVector B1 = UKismetMathLibrary::Matrix_TransformPosition(M1, CoverTriangles[i].pointB);
		FVector C1 = UKismetMathLibrary::Matrix_TransformPosition(M1, CoverTriangles[i].pointC);

		vertices.Add(A0);
		vertices.Add(C0);
		vertices.Add(B0);
		vertices.Add(A1);
		vertices.Add(B1);
		vertices.Add(C1);

		//Calculate triangle's normal
		FVector n0 = UKismetMathLibrary::Cross_VectorVector(B0 - A0, C0 - A0).GetSafeNormal();
		FVector n1 = UKismetMathLibrary::Cross_VectorVector(C1 - A1, B1 - A1).GetSafeNormal();

		normals.Add(n0);
		normals.Add(n0);
		normals.Add(n0);
		normals.Add(n1);
		normals.Add(n1);
		normals.Add(n1);
	}
	UpdateMeshSection(1, vertices, normals, UVs, color, tangents);
}

void USplineSweepMeshComponent::SweepPointsAlongSpline(USplineComponent* path, TArray<FVector>& PointsToSweep, TArray<FVector>& NormalsToSweep, int SegmentsNumber, float Rate, TArray<FVector>& OutPoints, TArray<FVector>& OutNormal, TArray<FVector2D>& OutUVs)
{
	//Clear array
	OutPoints.Empty();
	OutNormal.Empty();
	OutUVs.Empty();

	float SegmentLength = path->GetSplineLength()*Rate / SegmentsNumber;
	for (int i = 0; i < SegmentsNumber; i++)
	{
		FMatrix M = GetMatrixInSplineDistance(path, i * SegmentLength);
		for (int j = 0; j < PointsToSweep.Num(); j++)
		{
			//Transform position and normal
			OutPoints.Add(UKismetMathLibrary::Matrix_TransformPosition(M, PointsToSweep[j]));
			OutNormal.Add(UKismetMathLibrary::Matrix_TransformVector(M, NormalsToSweep[j]).GetSafeNormal());
			//Int to float to make "j / PointsToSweep.Num()" a float
			float fj = j;
			//Calculate UV,remap position into [0,1]
			OutUVs.Add(FVector2D(fj / PointsToSweep.Num(), i * SegmentLength / path->GetSplineLength()*Rate));
		}
	}

	//Add points at the end of path
	FMatrix M = GetMatrixInSplineDistance(path, path->GetSplineLength()*Rate);
	for (int j = 0; j < PointsToSweep.Num(); j++)
	{
		FVector point = UKismetMathLibrary::Matrix_TransformPosition(M, PointsToSweep[j]);
		OutPoints.Add(point);
		float fj = j;
		//At the end of path,UV should be(u,1)
		OutUVs.Add(FVector2D(fj / PointsToSweep.Num(), 1));
		OutNormal.Add(UKismetMathLibrary::Matrix_TransformVector(M, NormalsToSweep[j]).GetSafeNormal());
	}
}

TrianglePoints USplineSweepMeshComponent::FindAndRemoveFirstTriangleInSplineArea(TArray<FVector>& points)
{
	for (int i = 0; i < points.Num(); i++)
	{
		int a = i;
		int b = (i + 1) == points.Num() ? 0 : i + 1;
		int c = (i - 1) < 0 ? points.Num() - 1 : i - 1;

		bool isTriangle=false;
		//Use cross to make sure if triangle(a,b,c) is in the right side of spline
		if (UKismetMathLibrary::Cross_VectorVector((points[b] - points[a]), (points[c] - points[a])).X < 0)
		{
			isTriangle = true;
			//Check if other points in range of this triangle
			for (int j = 0; j < points.Num(); j++)
			{
				if (j != a && j != b && j != c)
				{
					bool b1 = UKismetMathLibrary::Cross_VectorVector((points[b] - points[a]), (points[j] - points[a])).X < 0;
					bool b2 = UKismetMathLibrary::Cross_VectorVector((points[a] - points[c]), (points[j] - points[c])).X < 0;
					bool b3 = UKismetMathLibrary::Cross_VectorVector((points[c] - points[b]), (points[j] - points[b])).X < 0;
					//if points[j] in range of triangle,this triangle should not be created 
					if (b1 && b2 && b3)
					{
						isTriangle = false;
						break;
					}
				}
			}
			if (isTriangle)
			{
				TrianglePoints retPoints = TrianglePoints(points[a], points[b], points[c]);
				//Remove a point and create new triangles with left point later
				points.RemoveAt(a);
				return retPoints;
			}
		}
	}
	//Preventing from infinity loop if none triangles can be created
	points.Empty();
	return TrianglePoints(FVector(), FVector(), FVector());
}

TArray<TrianglePoints> USplineSweepMeshComponent::ConvertSplineIntoTriangle(TArray<FVector>& points)
{
	TArray<TrianglePoints> triangles;
	//If <3,can not create triangle
	if (points.Num() < 3)
	{
		points.Empty();
	}
	//If =3,don not need to chek and itais a triangle
	else if (points.Num() == 3)
	{
		if (UKismetMathLibrary::Cross_VectorVector((points[0] - points[1]), (points[2] - points[1])).X > 0)
		triangles.Add(TrianglePoints(points[0], points[1], points[2]));
		points.Empty();
	}
	else
	{
		//Find first triangle and remove(in function"FindAndRemoveFirstTriangleInSplineArea"),and find next triangle recursion.Until no triangles can be created
		TrianglePoints triangle = FindAndRemoveFirstTriangleInSplineArea(points);
		triangles.Add(triangle);
		if (points.Num() >= 3)
		{
			TArray<TrianglePoints> temTriangles = ConvertSplineIntoTriangle(points);
			triangles.Append(temTriangles);
		}
	}
	return triangles;
}

TArray<FVector> USplineSweepMeshComponent::GetSplinePointsLocation(USplineComponent* spline)
{
	int number = spline->GetNumberOfSplinePoints();
	TArray<FVector> points;
	for (int i = 0; i < number; i++)
	{
		points.Add(spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
	}
	return points;
}

TArray<FVector> USplineSweepMeshComponent::GetSplinePointsNormal(USplineComponent* spline)
{
	int number = spline->GetNumberOfSplinePoints();
	TArray<FVector> normals;
	for (int i = 0; i < number; i++)
	{
		//Use direction to calculate normal of side surface
		normals.Add(UKismetMathLibrary::Cross_VectorVector(FVector(1, 0, 0), spline->GetDirectionAtSplinePoint(i, ESplineCoordinateSpace::Local)));
	}
	return normals;
}

FMatrix USplineSweepMeshComponent::GetMatrixInSplineDistance(USplineComponent* spline, float distance) const
{
	FVector X;
	FVector Z;
	FVector scale;

	//If distance is too long,use matrix in the end of path
	if (distance > spline->GetSplineLength())
	{
		int n = spline->GetNumberOfSplinePoints() - 1;
		X = spline->GetDirectionAtSplinePoint(n, ESplineCoordinateSpace::Local);
		Z = spline->GetUpVectorAtSplinePoint(n, ESplineCoordinateSpace::Local);

		scale = spline->GetScaleAtSplinePoint(n);
	}
	else
	{
		X = spline->GetDirectionAtDistanceAlongSpline(distance, ESplineCoordinateSpace::Local);
		Z = spline->GetUpVectorAtDistanceAlongSpline(distance, ESplineCoordinateSpace::Local);

		//Scale matrix
		scale = spline->GetScaleAtDistanceAlongSpline(distance);
	}
	FVector Y = UKismetMathLibrary::Cross_VectorVector(Z, X);
	Y *= scale.Y;
	Z *= scale.Z;

	FMatrix M = FMatrix(FPlane(X, 0), FPlane(Y, 0), FPlane(Z, 0), FPlane(spline->GetLocationAtDistanceAlongSpline(distance, ESplineCoordinateSpace::Local)));

	return M;
}

