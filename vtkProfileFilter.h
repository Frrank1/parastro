/*=========================================================================

  Program:   AstroViz plugin for ParaView
  Module:    $RCSfile: vtkProfileFilter.h,v $

  Copyright (c) Christine Corbett Moran
  All rights reserved.
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProfileFilter - shrink cells composing an arbitrary data set
// .SECTION Description
// Plots the mass function N(>M) as a scatter plot
// .SECTION See Also

#ifndef __vtkProfileFilter_h
#define __vtkProfileFilter_h

#include "vtkTableAlgorithm.h"
#include "vtkPointSet.h" // need for private function which takes this as arg

class VTK_EXPORT vtkProfileFilter : public vtkTableAlgorithm
{
public:
  static vtkProfileFilter* New();
  vtkTypeRevisionMacro(vtkProfileFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description:
  // Get/Set the density parameter
  vtkSetMacro(Delta, double);
  vtkGetMacro(Delta, double);
  // Description:
  // Get/Set the number of bins
  vtkSetMacro(NumberBins, int);
  vtkGetMacro(NumberBins, int);
  // Description:
  // Get/Set the number of bins
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

//BTX
protected:
  vtkProfileFilter();
  ~vtkProfileFilter();
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // This is called within ProcessRequest when a request asks the algorithm
  // to do its work. This is the method you should override to do whatever the
  // algorithm is designed to do. This happens during the fourth pass in the
  // pipeline execution process.
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
	// Set in GUI, with defaults
	double Delta;
	int NumberBins;
	double Center[3];
	// Set in constructor
	double BinLowerBound;
	double BinUpperBound;
	// TODO: implement
	double BinWidth;

private:
  vtkProfileFilter(const vtkProfileFilter&); // Not implemented
  void operator=(const vtkProfileFilter&); // Not implemented
	// Description:
	// from the set properties at input, set the BinLowerBound,
	// BinUpperBound, and BinWidth
	void GetBinInfo(vtkPointSet* input, double* binLowerBound, double* binUpperBound, double* binWidth);
	// Description:
	// From the properties BinLowerBound, BinUpperBound, and BinWidth
	// calculate the bin number this point belongs
	// to
	int GetBinNum(double point[],double binLowerBound,double binUpperBound,double binWidth);
//ETX
};

#endif

