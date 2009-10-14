#include "DataSetHelpers.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkSphereSource.h"
#include "vtkSmartPointer.h"
#include <cmath>

/*----------------------------------------------------------------------------
*
* Work with VtkPolyData (a derived class from vtkPointSet)
*
*---------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
vtkIdType SetPointValue(vtkPolyData* output,float pos[])
{
	vtkIdType id=output->GetPoints()->InsertNextPoint(pos);
	output->GetVerts()->InsertNextCell(1, &id);
	return id;
}

//----------------------------------------------------------------------------
float* DoublePointToFloat(double point[])
{
	float* floatPoint = new float[3];
	for(int i = 0; i < 3; ++i)
	{
		floatPoint[i]=static_cast<float>(point[i]);
	}
	return floatPoint;
}
//----------------------------------------------------------------------------
void CreateSphere(vtkPolyData* output,double radius,double center[])
{
	vtkSmartPointer<vtkSphereSource> sphere = \
	 															vtkSmartPointer<vtkSphereSource>::New();
	sphere->SetRadius(radius);
	sphere->SetCenter(center);
	sphere->Update();
	//Setting the points in the output to be those of the sphere
	output->SetPoints(sphere->GetOutput()->GetPoints());
	output->SetVerts(sphere->GetOutput()->GetVerts());
	output->SetPolys(sphere->GetOutput()->GetPolys());
}

/*----------------------------------------------------------------------------
*
* Work with VtkPointSet
*
*---------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
void AllocateDataArray(vtkPointSet* output, const char* arrayName,\
 			int numComponents, int numTuples)
{
	vtkSmartPointer<vtkFloatArray> dataArray=\
		vtkSmartPointer<vtkFloatArray>::New();
  	dataArray->SetNumberOfComponents(numComponents);
  	dataArray->SetName(arrayName);
		dataArray->SetNumberOfTuples(numTuples);
  output->GetPointData()->AddArray(dataArray);
}

//----------------------------------------------------------------------------
void AllocateDoubleDataArray(vtkPointSet* output, const char* arrayName,\
 			int numComponents, int numTuples)
{
	vtkSmartPointer<vtkDoubleArray> dataArray=\
		vtkSmartPointer<vtkDoubleArray>::New();

  	dataArray->SetNumberOfComponents(numComponents);
  	dataArray->SetName(arrayName);
		dataArray->SetNumberOfTuples(numTuples);
  output->GetPointData()->AddArray(dataArray);
}

//----------------------------------------------------------------------------
void InitializeDataArray(vtkDataArray* dataArray, const char* arrayName,\
 			int numComponents, int numTuples)
{
	dataArray->SetNumberOfComponents(numComponents);
	dataArray->SetName(arrayName);
	dataArray->SetNumberOfTuples(numTuples);
}
//----------------------------------------------------------------------------
double* GetPoint(vtkPointSet* output,vtkIdType id)
{
	double* nextPoint=new double[3]; 
	output->GetPoints()->GetPoint(id,nextPoint);
	return nextPoint;
}

//----------------------------------------------------------------------------
void SetDataValue(vtkPointSet* output, const char* arrayName,\
			vtkIdType id,float data[])
{
	output->GetPointData()->GetArray(arrayName)->SetTuple(id,data);
}

//----------------------------------------------------------------------------
void SetDataValue(vtkPointSet* output, const char* arrayName,\
			vtkIdType id,double data[])
{
	output->GetPointData()->GetArray(arrayName)->SetTuple(id,data);
}

//----------------------------------------------------------------------------
double* GetDataValue(vtkPointSet* output, const char* arrayName,\
 					vtkIdType id)
{
	double* data=new double[3];
	output->GetPointData()->GetArray(arrayName)->GetTuple(id,data);
	return data;
}


