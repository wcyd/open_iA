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

#include "iAModuleInterface.h"

class MdiChild;

class iASegmentationModuleInterface : public iAModuleInterface
{
	Q_OBJECT

public:
	void Initialize();

private slots:
	void binary_threshold();
	void otsu_Threshold_Filter();
	void maximum_Distance_Filter();
	void watershed_seg();
	void morph_watershed_seg();
	void adaptive_Otsu_Threshold_Filter();
	void rats_Threshold_Filter();
	void otsu_Multiple_Threshold_Filter();
	bool CalculateSegmentationMetrics();

private:
	//settings
	double btlower, btupper, btoutside, btinside; //binary threshold
	double otBins, otinside, otoutside;
	bool otremovepeaks;
	double mdfli, mdfbins; int mdfuli; //maximum distance filter parameters
	double wsLevel, wsThreshold;
	double mwsLevel; // Morphological Watershed Segmentation Filter
	bool mwsMarkWSLines, mwsFullyConnected; // Morphological Watershed Segmentation Filter
	double aotBins, aotOutside, aotInside, aotRadius; 
	unsigned int aotSamples, aotLevels, aotControlpoints;
	double rtPow, rtOutside, rtInside;
	double omtBins, omtThreshs, omtVe;
};
