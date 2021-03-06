/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include <QDialog>

class QLabel;

class vtkImageData;

class iAFoamCharacterizationTableAnalysis;

class iAFoamCharacterizationDialogAnalysis : public QDialog
{
		Q_OBJECT

	#ifndef M_PI
	#define M_PI 3.14159265359
	#endif

	public:
		explicit iAFoamCharacterizationDialogAnalysis(vtkImageData* _pImageData, QWidget* _pParent = nullptr);

	private:
		vtkImageData* m_pImageData = nullptr;

		QLabel* m_pLabel12 = nullptr;

		iAFoamCharacterizationTableAnalysis* m_pTable = nullptr;

		void analyse();

	private slots:
		void slotPushButtonOk();

	protected:
		virtual QSize sizeHint() const override;
};
