/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAAlgorithm.h"

enum iASmoothingType
{
	GRADIENT_ANISOTROPIC_DIFFUSION,
	CURVATURE_ANISOTROPIC_DIFFUSION,
	BILATERAL,
};

/**
 * Application of two edge preserving smoothing methods (itkGradientAnisotropicDiffusionImageFilter and itkCurvatureAnisotropicDiffusionImageFilter).
 * For further reference please look at http://www.itk.org/Doxygen/html/classitk_1_1GradientAnisotropicDiffusionImageFilter.html, 
 * http://www.itk.org/Doxygen/html/classitk_1_1CurvatureAnisotropicDiffusionImageFilter.html,
 * http://www.itk.org/Doxygen/html/classitk_1_1BilateralImageFilter.html
 */
class iAEdgePreservingSmoothing : public iAAlgorithm
{
public:
	iAEdgePreservingSmoothing( QString fn, iASmoothingType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	/**
	 * Sets smoothing parameters. 
	 * \param	i		NumberOfIterations. 
	 * \param	t		TimeStep. 
	 * \param	c		ConductanceParameter. 
	 */
	void setADParameters(unsigned int i, double t, double c) { iterations = i; timestep = t; conductance = c;};
	
	/**
	 * Sets smoothing parameters. 
	 * \param	r		rangesigma. 
	 * \param	d		domainsigma. 
	 */
	void setBParameters( double r, double d ) { rangesigma = r; domainsigma = d; };
protected:
	void run();
private:
	unsigned int iterations; 
	double timestep, conductance, rangesigma, domainsigma;
	iASmoothingType m_type;
	void gradientAnisotropicDiffusion();
	void curvatureAnisotropicDiffusion();
	void bilateral();
};
