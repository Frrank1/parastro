/*=========================================================================

		Program:   AstroViz plugin for ParaView
		Module:    $RCSfile: vtkFriendsOfFriendsHaloFinder.h,v $

		Copyright (c) Christine Corbett Moran
		All rights reserved.
   
	This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.


=========================================================================*/
// .NAME vtkFriendsOfFriendsHaloFinder - shrink cells composing an arbitrary data set
// .SECTION Description
// vtkFriendsOfFriendsHaloFinder 
 // Outline of this filter:
// 1. Build Kd tree
// 2. Go through each point in output
// 		o calculate points within linking length
// 		o these form a halo
// 3. Merge haloes
// 		o check each against each other if their bounds are linking length
//    apart. if so, merge haloes. Continue until every halo's bounds
//    have been checked against every other halo's
// .SECTION See Also
// vtkKdTree

#ifndef __vtkFriendsOfFriendsHaloFinder_h
#define __vtkFriendsOfFriendsHaloFinder_h
#include "vtkPointSetAlgorithm.h"

class vtkMultiProcessController;
class VTK_GRAPHICS_EXPORT vtkFriendsOfFriendsHaloFinder : public vtkPointSetAlgorithm
{
public:
  static vtkFriendsOfFriendsHaloFinder *New();
  vtkTypeRevisionMacro(vtkFriendsOfFriendsHaloFinder,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the linking length, for which any particle within this radius
	// of another particle is linked into a halo
  vtkSetMacro(LinkingLength, double);
  vtkGetMacro(LinkingLength, double);

  // Description:
  // Get/Set the minimum number of particles to consider a halo
  vtkSetMacro(MinimumNumberOfParticles, int);
  vtkGetMacro(MinimumNumberOfParticles, int);

  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
	// Groups all particles within a certain linking length of eachother into
	// a single halo. Considers particles to comprise a halo only if its group
	// has more than the requisite number of particles, as input by user. 
	// Output should contain the data set in which halos should be searched
	// before calling.
	int FindHaloes(vtkPointSet* output);

//BTX
protected:
  vtkFriendsOfFriendsHaloFinder();
  ~vtkFriendsOfFriendsHaloFinder();

  // Override to specify support for any vtkDataSet input type.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Main implementation.
  virtual int RequestData(vtkInformation*,
   	vtkInformationVector**,
    vtkInformationVector*);
  double LinkingLength;
	int MinimumNumberOfParticles;
  vtkMultiProcessController *Controller;

private:
  vtkFriendsOfFriendsHaloFinder(const vtkFriendsOfFriendsHaloFinder&);  // Not implemented.
  void operator=(const vtkFriendsOfFriendsHaloFinder&);  // Not implemented.
//ETX
};
#endif