//===========================================================================
/*
    CPSC 599.86 / 601.86 - Computer Haptics
    Winter 2017, University of Calgary

    This class encapsulates the visual and haptic rendering for an implicit
    surface.  It inherits the regular CHAI3D cMesh class, and taps into
    an external implementation of the Marching Cubes algorithm to create
    a triangle mesh from the implicit surface function.

    Your job is to implement the method that tracks the position of the
    proxy as the tool moves and interacts with the object, as described by
    the implicit surface rendering algorithm in Salisbury & Tarr 1997.

    \author    Your Name
*/
//===========================================================================

#ifndef IMPLICITMESH_H
#define IMPLICITMESH_H

#include "chai3d.h"

class ImplicitMesh : public chai3d::cMesh
{
  //! A visible sphere that tracks the position of the proxy on the surface
  chai3d::cShapeSphere m_projectedSphere;
  
  //! A pointer to the implicit function used to create this object
  double (*m_surfaceFunction)(double, double, double);

  // A pointer to the implicit function used to calculate the gradient
  chai3d::cVector3d (*m_gradientFunction)(double, double, double);

  // Get the closest point to the surface
  chai3d::cVector3d closestPointToSurface(chai3d::cVector3d);

  // Get the closest point to the tangent of the surface
  chai3d::cVector3d closestPointToTangent(chai3d::cVector3d);

  // Store the old tangent normal
  chai3d::cVector3d oldTangentNormal = chai3d::cVector3d(0,0,0);

  // Store if the device is moving on the surface of the object
  bool moving = false;

public:
  ImplicitMesh();
  virtual ~ImplicitMesh(); 

  double debugValue;

  //! Create a polygon mesh from an implicit surface function for visual rendering.
  void createFromFunction(double (*f)(double, double, double),
                          chai3d::cVector3d(*g)(double, double, double),
                          chai3d::cVector3d a_lowerBound,
                          chai3d::cVector3d a_upperBound,
                          double a_granularity);

  //! Contains code for graphically rendering this object in OpenGL.
  virtual void render(chai3d::cRenderOptions& a_options);

  //! Update the geometric relationship between the tool and the current object.
  virtual void computeLocalInteraction(const chai3d::cVector3d& a_toolPos,
                                       const chai3d::cVector3d& a_toolVel,
                                       const unsigned int a_IDN);
};

#endif
