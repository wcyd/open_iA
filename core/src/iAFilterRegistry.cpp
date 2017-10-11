/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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

#include "iAFilterRegistry.h"

void iAFilterRegistry::AddFilterFactory(QSharedPointer<iAAbstractFilterFactory> factory)
{
	m_filters.push_back(factory);
}

void iAFilterRegistry::AddFilterFactory(QSharedPointer<iAAbstractFilterFactory> factory, iAFilterRunCallback* callback)
{
	AddFilterFactory(factory);
	m_callback.insert(factory, callback);
}

QVector<QSharedPointer<iAAbstractFilterFactory>> const & iAFilterRegistry::FilterFactories()
{
	return m_filters;
}

iAFilterRunCallback* iAFilterRegistry::FilterCallback(QSharedPointer<iAAbstractFilterFactory> factory)
{
	return m_callback.contains(factory)? m_callback[factory] : nullptr;
}

QVector<QSharedPointer<iAAbstractFilterFactory>> iAFilterRegistry::m_filters;
QMap<QSharedPointer<iAAbstractFilterFactory>, iAFilterRunCallback*> iAFilterRegistry::m_callback;