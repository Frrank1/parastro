/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMassFunctionFilter.cxx,v $
=========================================================================*/
#include "vtkMassFunctionFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "astrovizhelpers/DataSetHelpers.h"
#include "vtkCellData.h"
#include "vtkTable.h"
#include <cmath>

vtkCxxRevisionMacro(vtkMassFunctionFilter, "$Revision: 1.72 $");
vtkStandardNewMacro(vtkMassFunctionFilter);

//----------------------------------------------------------------------------
vtkMassFunctionFilter::vtkMassFunctionFilter()
{
}

//----------------------------------------------------------------------------
vtkMassFunctionFilter::~vtkMassFunctionFilter()
{
}

//----------------------------------------------------------------------------
void vtkMassFunctionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkMassFunctionFilter::FillInputPortInformation(int, vtkInformation* info)
{
  // This filter uses the vtkDataSet cell traversal methods so it
  // suppors any data set type as input.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}




//----------------------------------------------------------------------------
int vtkMassFunctionFilter::RequestData(vtkInformation*,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  // Get input and output data.
  vtkPointSet* input = vtkPointSet::GetData(inputVector[0]);
  vtkTable* output = this->GetOutput();
	vtkSmartPointer<vtkDoubleArray> XArray=vtkSmartPointer<vtkDoubleArray>::New();
  	XArray->SetNumberOfComponents(1);
		XArray->SetNumberOfTuples(input->GetPoints()->GetNumberOfPoints());
		XArray->SetName("ids");
	vtkSmartPointer<vtkIntArray> dataValues=vtkSmartPointer<vtkIntArray>::New();
  	dataValues->SetNumberOfComponents(1);
		dataValues->SetNumberOfTuples(input->GetPoints()->GetNumberOfPoints());	
		dataValues->SetName("data values");		
	for(int nextPointId = 0;\
	 		nextPointId < input->GetPoints()->GetNumberOfPoints();\
	 		++nextPointId)
		{
		XArray->InsertValue(nextPointId,nextPointId);
		dataValues->InsertValue(nextPointId,pow(nextPointId,2));
		}
	// Updating the output
	output->AddColumn(XArray);
	output->AddColumn(dataValues);
	return 1;
}