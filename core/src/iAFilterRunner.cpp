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
#include "iAFilterRunner.h"

#include "iAFilter.h"

#include <QSettings>

#include "dlg_commoninput.h"
#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>

#include <QSharedPointer>
#include <QString>
#include <QTextDocument>

class iAFilter;

class vtkImageData;

iAFilterRunner::iAFilterRunner(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> paramValues, MdiChild* mdiChild) :
	iAAlgorithm(filter->Name(), mdiChild->getImagePointer(), mdiChild->getPolyData(), mdiChild->getLogger(), mdiChild),
	m_filter(filter),
	m_paramValues(paramValues)
{}

void iAFilterRunner::performWork()
{
	m_filter->SetUp(getConnector(), qobject_cast<MdiChild*>(parent())->getLogger(), getItkProgress());
	m_filter->Run(m_paramValues);
	emit workDone();
}

namespace
{
	QString SettingName(QSharedPointer<iAFilter> filter, QSharedPointer<iAAttributeDescriptor> param)
	{
		QString filterNameShort(filter->Name());
		filterNameShort.replace(" ", "");
		return QString("Filters/%1/%2/%3").arg(filter->Category()).arg(filterNameShort).arg(param->Name());
	}

	QString ValueTypePrefix(iAValueType val)
	{
		switch (val)
		{
		case Continuous : return "#"; // potentially ^ for DoubleSpinBox?
		case Discrete   : return "*";
		case Boolean    : return "$";
		case Categorical: return "+";
		default:
		case String: return "#";
		}
	}
}

iAFilterRunner* RunFilter(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	auto params = filter->Parameters();
	QSettings settings;
	QStringList dlgParamNames;
	QList<QVariant> dlgParamValues;
	for (auto param : params)
	{
		dlgParamNames << (ValueTypePrefix(param->ValueType()) + param->Name());
		if (param->ValueType() == Categorical)
		{
			QStringList comboValues = param->DefaultValue().toStringList();
			QString storedValue = settings.value(SettingName(filter, param), "").toString();
			for (int i = 0; i < comboValues.size(); ++i)
				if (comboValues[i] == storedValue)
					comboValues[i] = "!" + comboValues[i];
			dlgParamValues << comboValues;
		}
		else
		{
			dlgParamValues << settings.value(SettingName(filter, param), param->DefaultValue());
		}
	}
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(filter->Description());
	dlg_commoninput dlg(mainWnd, filter->Name(), dlgParamNames, dlgParamValues, fDescr);
	if (dlg.exec() != QDialog::Accepted)
		return nullptr;

	QMap<QString, QVariant> paramValues;
	int idx = 0;
	for (auto param : params)
	{
		QVariant value;
		switch (param->ValueType())
		{
		case Continuous:  value = dlg.getDblValue(idx);      break;
		case Discrete:    value = dlg.getIntValue(idx);      break;
		case String:      value = dlg.getText(idx);          break;
		case Boolean:     value = dlg.getCheckValue(idx);    break;
		case Categorical: value = dlg.getComboBoxValue(idx); break;
		}
		paramValues[param->Name()] = value;
		settings.setValue(SettingName(filter, param), value);
		++idx;
	}
	//! TODO: find way to check parameters already in dlg_commoninput (before closing)
	if (!filter->CheckParameters(paramValues))
	{
		return nullptr;
	}
	auto mdiChild = mainWnd->GetResultChild(filter->Name() + " " + mainWnd->activeMdiChild()->windowTitle());
	if (!mdiChild)
	{
		mainWnd->statusBar()->showMessage("Cannot get result child from main window!", 5000);
		return nullptr;
	}
	iAFilterRunner* thread = new iAFilterRunner(filter, paramValues, mdiChild);
	mdiChild->connectThreadSignalsToChildSlots(thread);
	mdiChild->addStatusMsg(filter->Name());
	mainWnd->statusBar()->showMessage(filter->Name(), 5000);
	thread->start();
	return thread;
}

QSharedPointer<iAFilter> iAFilterRunner::Filter()
{
	return m_filter;
}