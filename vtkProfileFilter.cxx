/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkProfileFilter.cxx,v $
=========================================================================*/
#include "vtkProfileFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "astrovizhelpers/DataSetHelpers.h"
#include "astrovizhelpers/ProfileHelpers.h"
#include "vtkCellData.h"
#include "vtkTable.h"
#include "vtkSortDataArray.h"
#include "vtkMath.h"
#include "vtkInformationDataObjectKey.h"
#include <cmath>
using vtkstd::string;

vtkCxxRevisionMacro(vtkProfileFilter, "$Revision: 1.72 $");
vtkStandardNewMacro(vtkProfileFilter);

//----------------------------------------------------------------------------
vtkProfileFilter::vtkProfileFilter()
{
  this->SetNumberOfInputPorts(2);
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("angular momentum",3,&ComputeAngularMomentum,TOTAL));
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("radial velocity",3,&ComputeRadialVelocity,TOTAL));
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("tangential velocity",3,&ComputeTangentialVelocity,
		TOTAL));
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("velocity squared",1,&ComputeVelocitySquared,TOTAL));
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("radial velocity squared",1,&ComputeRadialVelocitySquared,
		TOTAL));
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("tangential velocity squared",1,
		&ComputeTangentialVelocitySquared,TOTAL));
	// each of the following are postprocessed ProfileElements, computed only
	// at the end with the function given in the constructor taking two 
	// arguments based on the value of two columns computed along the way
	// as given by the two ProfileElement objects in the constructor
	ProfileElement cumulativeMass = ProfileElement("mass",1,0,CUMULATIVE);
	ProfileElement totalBinRadius =	ProfileElement("bin radius",1,0,TOTAL);
	this->AdditionalProfileQuantities.push_back(
		ProfileElement("circular velocity",1,
		&ComputeCircularVelocity,
		&cumulativeMass,&totalBinRadius));
	this->MaxR=1.0;
	this->Delta=0.0;
	this->BinNumber=30;
}

//----------------------------------------------------------------------------
vtkProfileFilter::~vtkProfileFilter()
{
/*	this->CumulativeQuantities->Delete();
	this->AdditionalProfileQuantities->Delete(); */ 
	// removed this--get	paraview(54834,0xa01ef500) malloc: *** error for object 0x21c8f8c0: incorrect checksum for freed object - object was probably modified after being freed. *** set a breakpoint in malloc_error_break to debug
}

//----------------------------------------------------------------------------
void vtkProfileFilter::PrintSelf(ostream& os, vtkIndent indent)
{
	// TODO: finish
  os << indent << "overdensity: "
     << this->Delta << "\n"
		 << indent << "bin number: "
     << this->BinNumber << "\n";
}

//----------------------------------------------------------------------------
void vtkProfileFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

int vtkProfileFilter::FillInputPortInformation (int port, 
                                                   vtkInformation *info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkProfileFilter::RequestData(vtkInformation *request,
																	vtkInformationVector **inputVector,
																	vtkInformationVector *outputVector)
{
	// Now we can get the input with which we want to work
 	vtkPolyData* dataSet = vtkPolyData::GetData(inputVector[0]);
	// Setting the center based upon the selection in the GUI
	vtkDataSet* pointInfo = vtkDataSet::GetData(inputVector[1]);
	vtkTable* const output = vtkTable::GetData(outputVector,0);
	output->Initialize();
	this->CalculateAndSetBounds(dataSet,pointInfo);
	// If we want to cut off at the virial radius, compute this, and remove the
	// portion of the data set we don't care about
	if(this->CutOffAtVirialRadius)
		{
		VirialRadiusInfo virialRadiusInfo = \
		 	ComputeVirialRadius(dataSet,this->Delta,this->MaxR,this->Center);
		vtkErrorMacro("virial radius is " << virialRadiusInfo.virialRadius);
		// note that if there was an error finding the virialRadius the 
		// radius returned is < 0
		//setting the dataSet to this newInput
		if(virialRadiusInfo.virialRadius>0)
			{
			dataSet = \
				GetDatasetWithinVirialRadius(virialRadiusInfo);	
			this->GenerateProfile(dataSet,output);
			dataSet->Delete();
			return 1;
			}
		else
			{
			vtkErrorMacro("Something has gone wrong with the virial radius finding. Perhaps change your delta, or your center, or if you are truely puzzled check out ProfileHelpers.cxx. For now binning out to the max radius instead of the virial.");
			}
		}
	this->GenerateProfile(dataSet,output);
	return 1;
}

//----------------------------------------------------------------------------
void vtkProfileFilter::CalculateAndSetBounds(vtkPolyData* input, 
	vtkDataSet* source)
{
	//TODO: this can later be done as in the XML documentation for this filter; 	  
	// for now, only getting the first point. this is the point selected in the
	// GUI, or the first end of the line selected in the GUI
	double* center = source->GetPoint(0);
	for(int i = 0; i < 3; ++i)
	{
		this->Center[i]=center[i];
	}
	// calculating the the max R
	this->MaxR=ComputeMaxR(input,this->Center);
	delete [] center;
}

//----------------------------------------------------------------------------
int vtkProfileFilter::GetBinNumber(double x[])
{
	double distanceToCenter = \
		sqrt(vtkMath::Distance2BetweenPoints(this->Center,x));
	return floor(distanceToCenter/this->BinSpacing);
}

//----------------------------------------------------------------------------
void vtkProfileFilter::GenerateProfile(vtkPolyData* input,vtkTable* output)
{	
	this->InitializeBins(input,output);
	this->ComputeStatistics(input,output);
}

//----------------------------------------------------------------------------
void vtkProfileFilter::InitializeBins(vtkPolyData* input,
	vtkTable* output)
{
	this->CalculateAndSetBinExtents(input,output);
	// always need this for averages
	AllocateDataArray(output,GetColumnName("number in bin",TOTAL).c_str(),
		1,this->BinNumber);
	AllocateDataArray(output,GetColumnName("number in bin",
		CUMULATIVE).c_str(),1,this->BinNumber);
	for(int i = 0; i < input->GetPointData()->GetNumberOfArrays(); ++i)
		{
		// Next array data
		vtkSmartPointer<vtkDataArray> nextArray = \
		 	input->GetPointData()->GetArray(i);
		int numComponents = nextArray->GetNumberOfComponents();
		string baseName = nextArray->GetName();
		AllocateDataArray(output,GetColumnName(baseName,TOTAL).c_str(),
				numComponents,this->BinNumber);
		AllocateDataArray(output,GetColumnName(baseName,AVERAGE).c_str(),
			numComponents,this->BinNumber);
		AllocateDataArray(output,GetColumnName(baseName,CUMULATIVE).c_str(),
			numComponents,this->BinNumber);
		}
	// For our additional quantities, allocating a column for the average 
	// and the sum. These are currently restricted to be 3-vectors
	for(int i = 0; 
		i < this->AdditionalProfileQuantities.size();
	 	++i)
		{
		ProfileElement nextElement = this->AdditionalProfileQuantities[i];
		AllocateDataArray(output,
			GetColumnName(nextElement.BaseName,
			nextElement.ProfileColumnType).c_str(),nextElement.NumberComponents,
			this->BinNumber);
		}
}

//----------------------------------------------------------------------------
void vtkProfileFilter::CalculateAndSetBinExtents(vtkPolyData* input,
	vtkTable* output)
{
	this->BinSpacing=this->MaxR/this->BinNumber;
	// the first column will be the bin radius
	string binRadiusColumnName=this->GetColumnName("bin radius",
		TOTAL);
	AllocateDataArray(output,binRadiusColumnName.c_str(),1,this->BinNumber);
	// setting the bin radii in the output
	for(int binNum = 0; binNum < this->BinNumber; ++binNum)
	{
	this->UpdateBin(binNum,SET,
		"bin radius",TOTAL,(binNum+1)*this->BinSpacing,output);
	}
}

//----------------------------------------------------------------------------
void vtkProfileFilter::ComputeStatistics(vtkPolyData* input,vtkTable* output)
{
	for(int nextPointId = 0;
	 		nextPointId < input->GetPoints()->GetNumberOfPoints();
	 		++nextPointId)
		{
			this->UpdateBinStatistics(input,nextPointId,output);
		}
	// Updating averages and doing relevant postprocessing
	this->BinAveragesAndPostprocessing(input,output);
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateBinStatistics(vtkPolyData* input,
 	vtkIdType pointGlobalId,vtkTable* output)
{
	double* x = GetPoint(input,pointGlobalId);
	// As we bin by radius always need
	double* r=PointVectorDifference(x,this->Center);
	// Many of the quantities explicitely require the velocity
	double* v=GetDataValue(input,"velocity",pointGlobalId);
	int binNum=this->GetBinNumber(x);
	this->UpdateBin(binNum,ADD,"number in bin",TOTAL,1.0,output);	
	this->UpdateCumulativeBins(binNum,ADD,
		"number in bin",CUMULATIVE,1.0,output);	
	assert(0<=binNum<=this->BinNumber);
	// Updating quanties for the input data arrays
	for(int i = 0; i < input->GetPointData()->GetNumberOfArrays(); ++i)
		{
		vtkSmartPointer<vtkDataArray> nextArray = \
		 	input->GetPointData()->GetArray(i);
		// getting the data for this point
		double* nextData = GetDataValue(input,
			nextArray->GetName(),pointGlobalId);
		// Updating the total bin
		string baseName = nextArray->GetName();
		this->UpdateBin(binNum,ADD,baseName,TOTAL,nextData, output);	
		this->UpdateBin(binNum,ADD,baseName,AVERAGE,nextData, output);	
		this->UpdateCumulativeBins(binNum,ADD,baseName,CUMULATIVE,nextData,
			output);
		//Finally some memory management
		delete [] nextData;
		}
	for(int i = 0; i < 
		this->AdditionalProfileQuantities.size(); 
		++i)
		{
		ProfileElement nextElement=this->AdditionalProfileQuantities[i];
		if(!nextElement.Postprocess)
			{
			double* additionalData = \
				nextElement.Function(v,r);
			this->UpdateBin(binNum,ADD,nextElement.BaseName,
				nextElement.ProfileColumnType,additionalData,output);
			}
		}
	// Finally some memory management
	delete [] x;
	delete [] r;
	delete [] v;
}

//----------------------------------------------------------------------------
void 	vtkProfileFilter::BinAveragesAndPostprocessing(
	vtkPolyData* input,vtkTable* output)
{
	for(int binNum = 0; binNum < this->BinNumber; ++binNum)
		{
		int binSize=output->GetValueByName(binNum,
			GetColumnName("number in bin",TOTAL).c_str()).ToInt();
		// For each input array, update its average column by getting
		// the total from the total column then dividing by 
		// the number in the bin. Only do this if the number in the bin
		// is greater than zero
			if(binSize>0)
				{
				vtkSmartPointer<vtkDataArray> nextArray;
				for(int i = 0; i < input->GetPointData()->GetNumberOfArrays(); ++i)
					{
					nextArray = input->GetPointData()->GetArray(i);
					string baseName = nextArray->GetName();
					this->UpdateBin(binNum,MULTIPLY,baseName,AVERAGE,1./binSize,output);
					}
				// For each additional quantity we also divide by N in the average
				// if requested
				for(int i = 0; 
					i < this->AdditionalProfileQuantities.size();
				 	++i)
					{
					ProfileElement nextElement = \
						this->AdditionalProfileQuantities[i];
					if(nextElement.ProfileColumnType==AVERAGE)
						{
						this->UpdateBin(binNum,MULTIPLY,nextElement.BaseName,
							AVERAGE,1./binSize,output);
						}
					}
				}

		// Finally post processing those items which are marked as such, don't
		// do this within the average loop as these quantities are allowed
		// to depend on averages themselves
		for(int i = 0; 
			i < this->AdditionalProfileQuantities.size();
		 	++i)
			{
			ProfileElement nextElement = \
				this->AdditionalProfileQuantities[i];
			if(nextElement.Postprocess)
				{
				vtkVariant argumentOne = \
		 			this->GetData(binNum,
					nextElement.PostProcessArgumentOne->BaseName,
					nextElement.PostProcessArgumentOne->ProfileColumnType,
					output);
				cout << "argument one " 
					<< nextElement.PostProcessArgumentOne->BaseName
					<< " type: " 
					<< nextElement.PostProcessArgumentOne->ProfileColumnType
					<< " value: " << argumentOne.ToDouble() << "\n";
				vtkVariant argumentTwo = \
					this->GetData(binNum,
					nextElement.PostProcessArgumentTwo->BaseName,
					nextElement.PostProcessArgumentTwo->ProfileColumnType,
					output);
				cout << "argument two " 
					<< nextElement.PostProcessArgumentTwo->BaseName
					<< " type: " 
					<< nextElement.PostProcessArgumentTwo->ProfileColumnType
					<< " value: " << argumentTwo.ToDouble() << "\n";
				double* updateData = \
					nextElement.PostProcessFunction(argumentOne,argumentTwo);
				this->UpdateBin(binNum,SET,nextElement.BaseName,TOTAL,
					updateData,output);
				cout << "update data " << updateData[0] << "\n\n";
				}
			}
		// TODO: add back later when I have the rest working
		// Computation and updating
		// TODO: these are scalars, but are currently being initialized
		// as vectors so all componenets are the same. make initialization
		// more general.
		/*
		this->UpdateBin(binNum,SET,"circular velocity",
			AVERAGE,cumulativeMass/binRadius,output);
		this->UpdateBin(binNum,SET,"density",AVERAGE,
			cumulativeMass/(4./3*vtkMath::Pi()*pow(binRadius,3)),output);
		// column data we need to compute dispersions
		vtkSmartPointer<vtkAbstractArray> vAve = \
			this->GetData(binNum,"velocity",AVERAGE,output).ToArray();
		vtkSmartPointer<vtkAbstractArray> vSquaredAve = \
		 	this->GetData(binNum,"velocity squared",AVERAGE,output).ToArray();
		vtkSmartPointer<vtkAbstractArray> vRadAve = \
			this->GetData(binNum,"radial velocity",AVERAGE,output).ToArray();
		vtkSmartPointer<vtkAbstractArray> vRadSquaredAve = \
			this->GetData(binNum,
			"radial velocity squared",AVERAGE,output).ToArray();
		vtkSmartPointer<vtkAbstractArray> vTanAve = \
			this->GetData(binNum,"tangential velocity",AVERAGE,output).ToArray();
		vtkSmartPointer<vtkAbstractArray> vTanSquaredAve = \
			this->GetData(binNum,
			"tangential velocity squared",AVERAGE,output).ToArray();
		*/

		// computing dispersions
		/*
		double* vDisp=ComputeVelocityDispersion(vSquaredAve,vAve);
		double* vRadDisp=ComputeVelocityDispersion(vRadSquaredAve,vRadAve);
		double* vTanDisp=ComputeVelocityDispersion(vTanSquaredAve,vTanAve);
		// updating output		
		this->UpdateBin(binNum,SET,"velocity dispersion",AVERAGE,
			coord,vDisp[coord],output);
		this->UpdateBin(binNum,SET,"radial velocity dispersion",AVERAGE,
			coord,vRadDisp[coord],output);
		this->UpdateBin(binNum,SET,"tangential velocity dispersion",AVERAGE,
			coord,vTanDisp[coord],output);
		// finally some memory management
		delete [] vDisp;
		delete [] vRadDisp;
		delete [] vTanDisp;
		*/
	}
}

//----------------------------------------------------------------------------
string vtkProfileFilter::GetColumnName(string baseName, 
	ColumnType columnType)
{
	switch(columnType)
		{
		case AVERAGE:
			return baseName+"_average";
		case TOTAL:
			return baseName+"_total";
		case CUMULATIVE:
			return baseName+"_cumulative";
		default:
			vtkDebugMacro("columnType not found for function GetColumnName, returning error string");
			return "error";
		}
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateBin(int binNum, BinUpdateType updateType,
 	string baseName, ColumnType columnType, double* updateData,
 	vtkTable* output)
{
	vtkVariant oldData = this->GetData(binNum,baseName,columnType,output);
	if(oldData.IsArray())
		{
		this->UpdateArrayBin(binNum,updateType,baseName,columnType,
			updateData,oldData.ToArray(),output);
		}
	else
		{
		this->UpdateDoubleBin(binNum,updateType,baseName,columnType,
			updateData[0],oldData.ToDouble(),output);
		}
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateBin(int binNum, BinUpdateType updateType,
 	string baseName, ColumnType columnType, double updateData,
 	vtkTable* output)
{
	vtkVariant oldData = this->GetData(binNum,baseName,columnType,output);
	if(oldData.IsArray())
		{
		int sizeOfUpdateData = oldData.ToArray()->GetNumberOfComponents();
		double* arrayUpdateData = new double[sizeOfUpdateData];
		for(int comp = 0; comp < sizeOfUpdateData; ++comp)
			{
			arrayUpdateData[comp] = updateData;
			}
		this->UpdateArrayBin(binNum,updateType,baseName,columnType,
			arrayUpdateData,oldData.ToArray(),output);
		// finally some memory management
		delete [] arrayUpdateData;
		}
	else
		{
		this->UpdateDoubleBin(binNum,updateType,baseName,columnType,
			updateData,oldData.ToDouble(),output);
		}
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateDoubleBin(int binNum, BinUpdateType updateType,
 	string baseName, ColumnType columnType, double updateData, double oldData,
 	vtkTable* output)
{
	switch(updateType)
		{
		case ADD:
			updateData+=oldData;
			break;
		case MULTIPLY:
			updateData*=oldData;
			break;
		case SET:
			break;
		}
		output->SetValueByName(binNum,
			GetColumnName(baseName,columnType).c_str(),updateData);
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateArrayBin(int binNum, BinUpdateType updateType,
 	string baseName, ColumnType columnType, double* updateData,
 	vtkAbstractArray* oldData, vtkTable* output)
{

	for(int comp = 0; comp < oldData->GetNumberOfComponents(); ++comp)
		{
		switch(updateType)
			{
			case ADD:
				oldData->InsertVariantValue(comp,
					updateData[comp]+oldData->GetVariantValue(comp).ToDouble());
				break;
			case MULTIPLY:
				oldData->InsertVariantValue(comp,
					updateData[comp]*oldData->GetVariantValue(comp).ToDouble());
				break;
			case SET:
				oldData->InsertVariantValue(comp,updateData[comp]);
				break;
			}
		}
	output->SetValueByName(binNum,GetColumnName(baseName,columnType).c_str(),
		oldData);
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateCumulativeBins(int binNum, BinUpdateType 		
	updateType, string baseName, ColumnType columnType, double* dataToAdd,
	vtkTable* output)
{
	for(int bin = binNum; bin < this->BinNumber; ++bin)
		{
		this->UpdateBin(bin,updateType,baseName,columnType,dataToAdd,output);
		}
}

//----------------------------------------------------------------------------
void vtkProfileFilter::UpdateCumulativeBins(int binNum, BinUpdateType 		
	updateType, string baseName, ColumnType columnType, double dataToAdd,
	vtkTable* output)
{
	for(int bin = binNum; bin < this->BinNumber; ++bin)
		{
		this->UpdateBin(bin,updateType,baseName,columnType,dataToAdd,output);
		}
}
//----------------------------------------------------------------------------
vtkVariant vtkProfileFilter::GetData(int binNum, string baseName,
	ColumnType columnType, vtkTable* output)
{
	return output->GetValueByName(binNum,
		GetColumnName(baseName,columnType).c_str());
}

//----------------------------------------------------------------------------
vtkProfileFilter::ProfileElement::ProfileElement(string baseName, 
	int numberComponents, double* (*funcPtr)(double [], double []),
	ColumnType columnType)
{
	this->BaseName = baseName;
	this->NumberComponents = numberComponents;
	this->Function = funcPtr;
	this->ProfileColumnType = columnType;
	this->Postprocess = 0;	
}

vtkProfileFilter::ProfileElement::ProfileElement(string baseName, 
	int numberComponents, double* (*funcPtr)(vtkVariant, vtkVariant),
	ProfileElement* postProcessArgumentOne, 
	ProfileElement* postProcessArgumentTwo)
{
	this->BaseName = baseName;
	this->NumberComponents = numberComponents;
	this->PostProcessFunction = funcPtr;
	this->ProfileColumnType = TOTAL;
	this->Postprocess = 1;
	this->PostProcessArgumentOne=postProcessArgumentOne;
	this->PostProcessArgumentTwo=postProcessArgumentTwo;
	
}

//----------------------------------------------------------------------------
vtkProfileFilter::ProfileElement::~ProfileElement()
{
	
}









