/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAVolumeRenderer.h"

#include "iAConsole.h"
#include "iAModalityTransfer.h"
#include "iAVolumeSettings.h"

#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

iAVolumeRenderer::iAVolumeRenderer(
	QSharedPointer<ModalityTransfer> transfer,
	vtkSmartPointer<vtkImageData> imgData)
:
	volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
	volume(vtkSmartPointer<vtkVolume>::New()),
	renderer(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	currentWindow(0)
{
	volProp->SetColor(0, transfer->getColorFunction());
	volProp->SetScalarOpacity(0, transfer->getOpacityFunction());
	volMapper->SetBlendModeToComposite();
	volMapper->SetInputData(imgData);
	volume->SetMapper(volMapper);
	volume->SetProperty(volProp);
	volume->SetVisibility(true);
	renderer->AddVolume(volume);
}

void iAVolumeRenderer::ApplySettings(iAVolumeSettings const & rs)
{
	volProp->SetAmbient(rs.AmbientLighting);
	volProp->SetDiffuse(rs.DiffuseLighting);
	volProp->SetSpecular(rs.SpecularLighting);
	volProp->SetSpecularPower(rs.SpecularPower);
	volProp->SetInterpolationType(rs.LinearInterpolation);
	volProp->SetShade(rs.Shading);
	volMapper->SetRequestedRenderMode(rs.Mode);
}

double * iAVolumeRenderer::GetOrientation()
{
	return volume->GetOrientation();
}

double * iAVolumeRenderer::GetPosition()
{
	return volume->GetPosition();
}

void iAVolumeRenderer::SetOrientation(double* orientation)
{
	volume->SetOrientation(orientation);
}

void iAVolumeRenderer::SetPosition(double* position)
{
	volume->SetPosition(position);
}

#include <vtkRendererCollection.h>

void iAVolumeRenderer::AddToWindow(vtkRenderWindow* w)
{
	if (currentWindow)
	{
		RemoveFromWindow();
	}
	w->AddRenderer(renderer);
	vtkCamera* cam = w->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	renderer->SetActiveCamera(cam);
	renderer->SetLayer(2);
	renderer->SetBackground(1, 0.5, 0.5);
	renderer->InteractiveOn();
	currentWindow = w;
}

void iAVolumeRenderer::RemoveFromWindow()
{
	if (!currentWindow)
	{
		DEBUG_LOG("RemoveFromWindow called on VolumeRenderer which was not attached to a window!\n");
		return;
	}
	currentWindow->RemoveRenderer(renderer);
	currentWindow = 0;
}


vtkSmartPointer<vtkVolume> iAVolumeRenderer::GetVolume()
{
	return volume;
}

void iAVolumeRenderer::Update()
{
	volume->Update();
	volMapper->Update();
}