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
// iA
#include <QTableView>

#include <vtkType.h>

class iABoneThickness;
class iABoneThicknessChartBar;

class iABoneThicknessTable : public QTableView
{
		Q_OBJECT

	public:
		explicit iABoneThicknessTable(QWidget* _pParent = nullptr);

		int selected() const;

		void set(iABoneThickness* _pBoneThickness, iABoneThicknessChartBar* _pBoneThicknessChartBar);
		void setSelected(const vtkIdType& _idSelected);

	private:
		iABoneThickness* m_pBoneThickness = nullptr;
		iABoneThicknessChartBar* m_pBoneThicknessChartBar = nullptr;

	protected:
		virtual QSize minimumSizeHint() const override;
		virtual void mousePressEvent(QMouseEvent* e) override;
		virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

};
