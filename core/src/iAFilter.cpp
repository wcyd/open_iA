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
#include "iAFilter.h"

#include "iALogger.h"
#include "iAAttributeDescriptor.h"

iAFilter::iAFilter(QString const & name, QString const & category, QString const & description):
	m_name(name),
	m_category(category),
	m_description(description)
{}

iAFilter::~iAFilter()
{}

QString iAFilter::Name() const
{
	return m_name;
}

QString iAFilter::Category() const
{
	int slashPos = m_category.indexOf("/");
	return slashPos > 0 ? m_category.left(slashPos) : m_category;
}

QString iAFilter::FullCategory() const
{
	return m_category;
}

QString iAFilter::Description() const
{
	return m_description;
}

QVector<pParameter> const & iAFilter::Parameters() const
{
	return m_parameters;
}

void iAFilter::SetUp(iAConnector* con, iALogger* log, iAProgress* progress)
{
	m_con = con;
	m_log = log;
	m_progress = progress;
}

bool iAFilter::CheckParameters(QMap<QString, QVariant> parameters)
{
	return true;
}

void iAFilter::AddMsg(QString msg)
{
	m_log->log(msg);
}

void iAFilter::AddParameter(QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	m_parameters.push_back(iAAttributeDescriptor::CreateParam(name, valueType, defaultValue, min, max));
}
