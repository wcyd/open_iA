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
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "iAIOProvider.h"

#include <QObject>

const QString iAIOProvider::ProjectFileExtension(".mod");
const QString iAIOProvider::ProjectFileTypeFilter("open_iA project file (*"+ProjectFileExtension+");;All files (*.*)");

QString iAIOProvider::GetSupportedLoadFormats()
{
	return QString(
		"All supported types (*.mhd *.mha *.stl *.vgi *.raw *.rec *.vol *.pro *.pars *.dcm *.nrrd *.oif *.am *.vti *.bmp *.jpg *.jpeg *.png *.tif *.tiff);;"
		"Meta Images (*.mhd *.mha);;"
		"STL files (*.stl);;"
		"VGI files (*.vgi);;"
		"RAW files (*.raw *.rec *.vol);;"
		"PRO files (*.pro);;"
		"PARS files (*.pars);;"
		"Dicom Series (*.dcm);;"
		"NRRD files (*.nrrd);;"
		"Olympus FluoView (*.oif);;"
		"AmiraMesh (*.am);;"
		"VTI files (*.vti);;") +
		GetSupportedImageFormats();
}


QString iAIOProvider::GetSupportedSaveFormats()
{
	return QObject::tr(
		"MetaImages (*.mhd *.mha );;"
		"STL files (*.stl);;"
		"AmiraMesh (*.am)"
	);
}

QString iAIOProvider::GetSupportedImageStackFormats()
{
	return QString("All supported types (*.mhd *.mha *.tif *.png *.jpg *.bmp);;"
		"MetaImages (*.mhd *.mha);;")+
		GetSupportedImageFormats();
}

QString iAIOProvider::GetSupportedVolumeStackFormats()
{
	return QObject::tr(
		"All supported types (*.mhd *.raw *.volstack);;"
		"MetaImages (*.mhd *.mha);;"
		"RAW files (*.raw);;"
		"Volume Stack (*.volstack);;"
	);
}

QString iAIOProvider::GetSupportedImageFormats()
{
	return QObject::tr(
		"BMP (*.bmp)"
		"JPEG (*.jpg);;"
		"PNG (*.png);;"
		"TIFF (*.tif);;"
	);
}