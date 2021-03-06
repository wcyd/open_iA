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
#include "pch.h"
#include "iAModalityTransfer.h"

#include "charts/iAHistogramData.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <cassert>

iAModalityTransfer::iAModalityTransfer(double range[2]):
	m_tfInitialized(false)
{
	m_ctf = GetDefaultColorTransferFunction(range);
	m_otf = GetDefaultPiecewiseFunction(range, true);
}

void iAModalityTransfer::ComputeStatistics(vtkSmartPointer<vtkImageData> img)
{
	if (m_tfInitialized)	// already calculated
		return;
	m_ctf = GetDefaultColorTransferFunction(img->GetScalarRange()); // Set range of rgb, rgba or vector pixel type images to fully opaque
	m_otf = GetDefaultPiecewiseFunction(img->GetScalarRange(), img->GetNumberOfScalarComponents() == 1);
	m_tfInitialized = true;
}

void iAModalityTransfer::Reset()
{
	m_tfInitialized = false;
	m_histogramData.clear();
}

void iAModalityTransfer::ComputeHistogramData(vtkSmartPointer<vtkImageData> imgData, size_t binCount)
{
	if (imgData->GetNumberOfScalarComponents() != 1 || (m_histogramData && m_histogramData->GetNumBin() == binCount))
		return;
	m_histogramData = iAHistogramData::Create(imgData, binCount, &m_imageInfo);
}

QSharedPointer<iAHistogramData> const iAModalityTransfer::GetHistogramData() const
{
	return m_histogramData;
}

vtkPiecewiseFunction* iAModalityTransfer::GetOpacityFunction()
{
	assert(m_otf);
	return m_otf;
}

vtkColorTransferFunction* iAModalityTransfer::GetColorFunction()
{
	assert(m_ctf);
	return m_ctf;
}

iAImageInfo const & iAModalityTransfer::Info() const
{
	// TODO: make sure image info is initialzed!
	return m_imageInfo;
}
