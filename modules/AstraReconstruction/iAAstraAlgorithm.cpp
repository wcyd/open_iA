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
#include "iAAstraAlgorithm.h"

#include "dlg_ProjectionParameters.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"
#include "iAvec3.h"
#include "mainwindow.h"
#include "mdichild.h"

#define ASTRA_CUDA
#include <astra/CudaBackProjectionAlgorithm3D.h>
#include <astra/CudaFDKAlgorithm3D.h>
#include <astra/CudaCglsAlgorithm3D.h>
#include <astra/CudaSirtAlgorithm3D.h>
#include <astra/CudaForwardProjectionAlgorithm3D.h>
#include <astra/CudaProjector3D.h>

#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <QMessageBox>
#include <QtMath>  // for qDegreesToRadians

#include <cuda_runtime_api.h>

namespace
{
	QString linspace(double projAngleStart, double projAngleEnd, int projAnglesCount)
	{
		QString result;
		for (int i = 0; i <= projAnglesCount - 2; i++)
		{
			double temp = projAngleStart + i*(projAngleEnd - projAngleStart) / (floor((double)projAnglesCount) - 1);
			result.append(QString::number(temp) + ",");
		}
		result.append(QString::number(projAngleEnd));
		return result;
	}

	void CreateConeProjGeom(astra::Config & projectorConfig, QMap<QString, QVariant> const & parameters)
	{
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone");
		projGeomNode.addChildNode("DetectorSpacingX", parameters["Detector Spacing X"].toDouble());
		projGeomNode.addChildNode("DetectorSpacingY", parameters["Detector Spacing Y"].toDouble());
		projGeomNode.addChildNode("DetectorRowCount", parameters["Detector Row Count"].toDouble());
		projGeomNode.addChildNode("DetectorColCount", parameters["Detector Column Count"].toDouble());
		projGeomNode.addChildNode("ProjectionAngles", linspace(
			qDegreesToRadians(parameters["Projection Angle Start"].toDouble()),
			qDegreesToRadians(parameters["Projection Angle End"].toDouble()),
			parameters["Projection Angle Count"].toUInt()).toStdString());
		projGeomNode.addChildNode("DistanceOriginDetector", parameters["Distance Origin-Detector"].toDouble());
		projGeomNode.addChildNode("DistanceOriginSource",   parameters["Distance Origin-Source"].toDouble());
	}


	void CreateConeVecProjGeom(astra::Config & projectorConfig, QMap<QString, QVariant> const & parameters)
	{
		QString vectors;
		for (int i = 0; i<parameters["Projection Angle Count"].toUInt(); ++i)
		{
			double curAngle = qDegreesToRadians(parameters["Projection Angle Start"].toDouble()) +
				i*(qDegreesToRadians(parameters["Projection Angle End"].toDouble())
					- qDegreesToRadians(parameters["Projection Angle Start"].toDouble())) /
				(parameters["Projection Angle Count"].toUInt() - 1);
			iAVec3 sourcePos(
				sin(curAngle) * parameters["Distance Origin-Source"].toDouble(),
				-cos(curAngle) * parameters["Distance Origin-Source"].toDouble(),
				0);
			iAVec3 detectorCenter(
				-sin(curAngle) * parameters["Distance Origin-Detector"].toDouble(),
				cos(curAngle) * parameters["Distance Origin-Detector"].toDouble(),
				0);
			iAVec3 detectorPixelHorizVec(				// vector from detector pixel(0, 0) to(0, 1)
				cos(curAngle) * parameters["Detector Spacing X"].toDouble(),
				sin(curAngle) * parameters["Detector Spacing X"].toDouble(),
				0);
			iAVec3 detectorPixelVertVec(0, 0, parameters["Detector Spacing Y"].toDouble()); // vector from detector pixel(0, 0) to(1, 0)
			iAVec3 shiftVec = detectorPixelHorizVec.normalize() * parameters["Center of Rotation Offset"].toDouble();
			sourcePos += shiftVec;
			detectorCenter += shiftVec;

			if (!vectors.isEmpty()) vectors += ",";
			vectors += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12")
				.arg(sourcePos.x).arg(sourcePos.y).arg(sourcePos.z)
				.arg(detectorCenter.x).arg(detectorCenter.y).arg(detectorCenter.z)
				.arg(detectorPixelHorizVec.x).arg(detectorPixelHorizVec.y).arg(detectorPixelHorizVec.z)
				.arg(detectorPixelVertVec.x).arg(detectorPixelVertVec.y).arg(detectorPixelVertVec.z);
		}
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone_vec");
		projGeomNode.addChildNode("DetectorRowCount", parameters["Detector Row Count"].toDouble());
		projGeomNode.addChildNode("DetectorColCount", parameters["Detector Column Count"].toDouble());
		projGeomNode.addChildNode("Vectors", vectors.toStdString());
	}

	void FillVolumeGeometryNode(astra::XMLNode & volGeomNode, QMap<QString, QVariant> const & parameters, double const volSpacing[3])
	{
		volGeomNode.addChildNode("GridColCount",   parameters["Volume Dimension X"].toDouble());      // columns are "y-direction" (second index component in buffer) in astra
		volGeomNode.addChildNode("GridRowCount",   parameters["Volume Dimension Y"].toDouble());      // rows are "x-direction" (first index component in buffer) in astra
		volGeomNode.addChildNode("GridSliceCount", parameters["Volume Dimension Z"].toDouble());
		astra::XMLNode winMinXOption = volGeomNode.addChildNode("Option");
		winMinXOption.addAttribute("key", "WindowMinX");
		winMinXOption.addAttribute("value", -parameters["Volume Dimension Y"].toDouble() * volSpacing[1] / 2.0);
		astra::XMLNode winMaxXOption = volGeomNode.addChildNode("Option");
		winMaxXOption.addAttribute("key", "WindowMaxX");
		winMaxXOption.addAttribute("value", parameters["Volume Dimension Y"].toDouble() * volSpacing[1] / 2.0);
		astra::XMLNode winMinYOption = volGeomNode.addChildNode("Option");
		winMinYOption.addAttribute("key", "WindowMinY");
		winMinYOption.addAttribute("value", -parameters["Volume Dimension X"].toDouble() * volSpacing[0] / 2.0);
		astra::XMLNode winMaxYOption = volGeomNode.addChildNode("Option");
		winMaxYOption.addAttribute("key", "WindowMaxY");
		winMaxYOption.addAttribute("value", parameters["Volume Dimension X"].toDouble() * volSpacing[0] / 2.0);
		astra::XMLNode winMinZOption = volGeomNode.addChildNode("Option");
		winMinZOption.addAttribute("key", "WindowMinZ");
		winMinZOption.addAttribute("value", -parameters["Volume Dimension Z"].toDouble() * volSpacing[2] / 2.0);
		astra::XMLNode winMaxZOption = volGeomNode.addChildNode("Option");
		winMaxZOption.addAttribute("key", "WindowMaxZ");
		winMaxZOption.addAttribute("value", parameters["Volume Dimension Z"].toDouble() * volSpacing[2] / 2.0);
	}


	template <typename T>
	void SwapXYandCastToFloat(vtkSmartPointer<vtkImageData> img, astra::float32* buf)
	{
		T* imgBuf = static_cast<T*>(img->GetScalarPointer());
		int * dim = img->GetDimensions();
		size_t inIdx = 0;
		for (int z = 0; z < dim[2]; ++z)
		{
			for (int y = 0; y < dim[1]; ++y)
			{
#pragma omp parallel for
				for (int x = 0; x < dim[0]; ++x)

				{
					size_t outIdx = y + ((x + z * dim[0]) * dim[1]);
					buf[outIdx] = static_cast<float>(imgBuf[inIdx + x]);
				}
				inIdx += dim[0];
			}
		}
	}


	bool IsCUDAAvailable()
	{
		int deviceCount = 0;
		cudaGetDeviceCount(&deviceCount);
		if (deviceCount == 0)
			return false;
		/*
		// TODO: Allow choosing a device to use?
		else
		{
		for (int dev = 0; dev < deviceCount; dev++)
		{
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, dev);
		DEBUG_LOG(QString("%1. Compute Capability: %2.%3. Clock Rate (kHz): %5. Memory Clock Rate (kHz): %6. Memory Bus Width (bits): %7. Concurrent kernels: %8. Total memory: %9.")
		.arg(deviceProp.name)
		.arg(deviceProp.major)
		.arg(deviceProp.minor)
		.arg(deviceProp.clockRate)
		.arg(deviceProp.memoryClockRate)
		.arg(deviceProp.memoryBusWidth)
		.arg(deviceProp.concurrentKernels)
		.arg(deviceProp.totalGlobalMem)
		);
		}
		}
		*/
		return true;
	}

	QStringList AlgorithmStrings()
	{
		static QStringList algorithms;
		if (algorithms.empty())
			algorithms << "BP" << "FDK" << "SIRT" << "CGLS";
		return algorithms;
	}

	int MapAlgoStringToIndex(QString const & algo)
	{
		if (AlgorithmStrings().indexOf(algo) == -1)
		{
			DEBUG_LOG("Invalid Algorithm Type selection!");
			return iAASTRAReconstruct::FDK3D;
		}
		return AlgorithmStrings().indexOf(algo);
	}

	QString MapAlgoIndexToString(int astraIndex)
	{
		if (astraIndex < 0 || astraIndex >= AlgorithmStrings().size())
		{
			DEBUG_LOG("Invalid Algorithm Type selection!");
			return "Invalid";
		}
		return AlgorithmStrings()[astraIndex];
	}

	void AddCommonForwardReconstructParams(iAFilter* filter)
	{
		QStringList projectionGeometries; projectionGeometries << "cone";
		filter->AddParameter("Projection Geometry", Categorical, projectionGeometries);
		filter->AddParameter("Detector Spacing X", Continuous, 1.0);
		filter->AddParameter("Detector Spacing Y", Continuous, 1.0);
		filter->AddParameter("Projection Angle Start", Continuous, 0.0);
		filter->AddParameter("Projection Angle End", Continuous, 359.0);
		filter->AddParameter("Distance Origin-Detector", Continuous, 1.0);
		filter->AddParameter("Distance Source-Origin", Continuous, 1.0);
	}
}


IAFILTER_CREATE(iAASTRAForwardProject)


iAASTRAForwardProject::iAASTRAForwardProject() :
	iAFilter("ASTRA Forward Projection", "ASTRA Toolbox", "Forward Projection with the ASTRA Toolbox")
{
	AddCommonForwardReconstructParams(this);
	AddParameter("Detector Row Count", Discrete, 512);
	AddParameter("Detector Row Count", Discrete, 512);
	AddParameter("Projection Angle Count", Discrete, 360);
}


void iAASTRAForwardProject::Run(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkImageData> img = m_con->GetVTKImage();
	int * dim = img->GetDimensions();
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	/*
	// further (optional) "Option"s (as GPUIndex):
	"ProjectionKernel"
	"VoxelSuperSampling"
	"DetectorSuperSampling"
	"DensityWeighting"
	*/
	CreateConeProjGeom(projectorConfig, parameters);

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	FillVolumeGeometryNode(volGeomNode, parameters, img->GetSpacing());

	astra::float32* buf = new astra::float32[dim[0] * dim[1] * dim[2]];
	VTK_TYPED_CALL(SwapXYandCastToFloat, img->GetScalarType(), img, buf);

	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry());
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), buf);
	astra::CCudaForwardProjectionAlgorithm3D* algorithm = new astra::CCudaForwardProjectionAlgorithm3D();
	algorithm->initialize(projector, projectionData, volumeData);
	algorithm->run();

	int projDim[3] = { parameters["Detector Columne Count"].toInt(), parameters["Detector Row Count"].toInt(),
		parameters["Projection Angle Count"].toInt() };
	double projSpacing[3] = {
		parameters["Detector Spacing X"].toDouble(),
		parameters["Detector Spacing Y"].toDouble(),
		// "normalize" z spacing with projections count to make sinograms with different counts more easily comparable:
		parameters["Detector Spacing X"].toDouble() * 180 / parameters["Projection Angle Count"].toDouble() };
	auto projImg = AllocateImage(VTK_FLOAT, projDim, projSpacing);
	float* projImgBuf = static_cast<float*>(projImg->GetScalarPointer());
	astra::float32*** projData3D = projectionData->getData3D();
	size_t imgIndex = 0;
	unsigned int projAngleCount = parameters["Projection Angle Count"].toUInt();
	unsigned int detectorColCnt = parameters["Detector Columne Count"].toUInt();
	for (int z = 0; z < projDim[2]; ++z)
	{
		for (int y = 0; y < projDim[1]; ++y)
		{
#pragma omp parallel for
			for (int x = 0; x < projDim[0]; ++x)
			{
				projImgBuf[imgIndex + x] = projData3D[y][projAngleCount - z - 1][detectorColCnt - x - 1];
			}
			imgIndex += projDim[0];
		}
	}
	m_con->SetImage(projImg);
	m_con->Modified();

	delete[] buf;
	delete algorithm;
	delete volumeData;
	delete projectionData;
	delete projector;
}



IAFILTER_CREATE(iAASTRAReconstruct)


iAASTRAReconstruct::iAASTRAReconstruct() :
	iAFilter("ASTRA Reconstruction", "ASTRA Toolbox", "Reconstruction with the ASTRA Toolbox")
{
	AddCommonForwardReconstructParams(this);
	AddParameter("Detector Row Dimension", Discrete, 1);
	AddParameter("Detector Row Dimension", Discrete, 0);
	AddParameter("Projection Angle Dimension", Discrete, 2);

	AddParameter("Volume Dimension X", Discrete, 512);
	AddParameter("Volume Dimension Y", Discrete, 512);
	AddParameter("Volume Dimension Z", Discrete, 512);
	AddParameter("Volume Spacing X", Continuous, 1.0);
	AddParameter("Volume Spacing Y", Continuous, 1.0);
	AddParameter("Volume Spacing Z", Continuous, 1.0);

	AddParameter("Algorithm Type", Categorical, AlgorithmStrings());

	AddParameter("Number of Iterations", Discrete, 100);
	AddParameter("Center of Rotation Correction", Boolean, false);
	AddParameter("Center of Rotation Offset", Continuous, 0.0);
}


template <typename T>
void SwapDimensions(vtkSmartPointer<vtkImageData> img, astra::float32* buf, int detColDim, int detRowDim, int projAngleDim)
{
	float* imgBuf = static_cast<float*>(img->GetScalarPointer());
	int * dim = img->GetDimensions();
	int detColDimIdx = detColDim % 3;		// only do modulus once before loop
	int detRowDimIdx = detRowDim % 3;
	int projAngleDimIdx = projAngleDim % 3;
	int idx[3];
	size_t imgBufIdx = 0;
	for (idx[2] = 0; idx[2] < dim[2]; ++idx[2])
	{
		for (idx[1] = 0; idx[1] < dim[1]; ++idx[1])
		{
#pragma omp parallel for
			for (idx[0] = 0; idx[0] < dim[0]; ++idx[0])
			{
				int detCol    = idx[detColDimIdx];     if (detColDim >= 3)    { detCol    = dim[detColDimIdx]    - detCol    - 1; }
				int detRow    = idx[detRowDimIdx];     if (detRowDim >= 3)    { detRow    = dim[detRowDimIdx]    - detRow    - 1; }
				int projAngle = idx[projAngleDimIdx];  if (projAngleDim >= 3) { projAngle = dim[projAngleDimIdx] - projAngle - 1; }
				int bufIndex = detCol + ((projAngle + detRow*dim[projAngleDimIdx])*dim[detColDimIdx]);
				buf[bufIndex] = static_cast<float>(imgBuf[imgBufIdx + idx[0]]);
			}
			imgBufIdx += dim[0];
		}
	}
}


void iAASTRAReconstruct::Run(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkImageData> img = m_con->GetVTKImage();
	int * dim = img->GetDimensions();

	astra::float32* buf = new astra::float32[dim[0] * dim[1] * dim[2]];
	//VTK_TYPED_CALL(SwapDimensions, img->GetScalarType(), img, buf, m_detColDim, m_detRowDim, m_projAngleDim, m_detRowCnt, m_detColCnt, m_projAnglesCount);
	VTK_TYPED_CALL(SwapDimensions, img->GetScalarType(), img, buf,
		parameters["Detector Column Dimension"].toUInt(),
		parameters["Detector Row Dimension"].toUInt(),
		parameters["Projection Angle Dimension"].toUInt());

	// create XML configuration:
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	assert(parameters["Projection Geometry"].toString() == "cone");
	if (parameters["Center of Rotation Correction"].toBool())
	{
		CreateConeVecProjGeom(projectorConfig, parameters);
	}
	else
	{
		CreateConeProjGeom(projectorConfig, parameters);
	}
	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	double volSpacing[3] = {
		parameters["Volume Spacing X"].toDouble(),
		parameters["Volume Spacing Y"].toDouble(),
		parameters["Volume Spacing Z"].toDouble()
	};
	FillVolumeGeometryNode(volGeomNode, parameters, volSpacing);

	// create Algorithm and run:
	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), buf);
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), 0.0f);
	switch (MapAlgoStringToIndex(parameters["Algorithm Type"].toString()))
	{
		case BP3D: {
			astra::CCudaBackProjectionAlgorithm3D* bp3dalgo = new astra::CCudaBackProjectionAlgorithm3D();
			bp3dalgo->initialize(projector, projectionData, volumeData);		// unfortunately, initialize is not virtual in CReconstructionAlgorithm3D, otherwise we could call it once after the switch...
			bp3dalgo->run();
			delete bp3dalgo;
			break;
		}
		case FDK3D: {
			astra::CCudaFDKAlgorithm3D* fdkalgo = new astra::CCudaFDKAlgorithm3D();
			fdkalgo->initialize(projector, projectionData, volumeData);
			fdkalgo->run();
			delete fdkalgo;
			break;
		}
		case SIRT3D: {
			astra::CCudaSirtAlgorithm3D* sirtalgo = new astra::CCudaSirtAlgorithm3D();
			sirtalgo->initialize(projector, projectionData, volumeData);
			sirtalgo->run(parameters["Number of Iterations"].toInt());
			delete sirtalgo;
			break;
		}
		case CGLS3D: {
			astra::CCudaCglsAlgorithm3D* cglsalgo = new astra::CCudaCglsAlgorithm3D();
			cglsalgo->initialize(projector, projectionData, volumeData);
			cglsalgo->run(parameters["Number of Iterations"].toInt());
			delete cglsalgo;
			break;
		}
		default:
			DEBUG_LOG("Unknown reconstruction algorithm selected!");
	}

	// retrieve result image:
	int volDim[3] = {
		parameters["Volume Dimension X"].toInt(),
		parameters["Volume Dimension Y"].toInt(),
		parameters["Volume Dimension Z"].toInt()
	};
	auto volImg = AllocateImage(VTK_FLOAT, volDim, volSpacing);
	float* volImgBuf = static_cast<float*>(volImg->GetScalarPointer());
	astra::float32*** volData3D = volumeData->getData3D();
	size_t imgIndex = 0;
	for (int z = 0; z < volDim[2]; ++z)
	{
		for (int y = 0; y < volDim[1]; ++y)
		{
			#pragma omp parallel for
			for (int x = 0; x < volDim[0]; ++x)
			{
				volImgBuf[imgIndex + x] = volData3D[z][x][y];
			}
			imgIndex += volDim[0];
		}
	}
	m_con->SetImage(volImg);
	m_con->Modified();

	delete[] buf;
	delete volumeData;
	delete projectionData;
	delete projector;
}



void iAASTRAFilterRunner::Run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	if (!IsCUDAAvailable())
	{
		QMessageBox::warning(mainWnd, "ASTRA", "No CUDA device available. ASTRA toolbox operations require a CUDA-capable device.");
		return;
	}
	iAFilterRunnerGUI::Run(filter, mainWnd);
}

bool iAASTRAFilterRunner::AskForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & parameters, MainWindow* mainWnd)
{
	dlg_ProjectionParameters dlg;
	dlg.setWindowTitle(filter->Name());
	if (filter->Name() == "ASTRA Forward Projection")
	{
		dlg.fillProjectionGeometryValues(
			parameters["Projection Geometry"]   .toString(),
			parameters["Detector Spacing X"]    .toDouble(),
			parameters["Detector Spacing Y"]    .toDouble(),
			parameters["Detector Row Count"]    .toUInt(),
			parameters["Detector Column Count"] .toUInt(),
			parameters["Projection Angle Start"].toDouble(),
			parameters["Projection Angle End"]  .toDouble(),
			parameters["Projection Angle Count"].toUInt(),
			parameters["Distance Origin-Detector"].toDouble(),
			parameters["Distance Origin-Source"].toDouble());
	}
	else
	{
		dlg.fillProjectionGeometryValues(
			parameters["Projection Geometry"].toString(),
			parameters["Detector Spacing X"].toDouble(),
			parameters["Detector Spacing Y"].toDouble(),
			parameters["Projection Angle Start"].toDouble(),
			parameters["Projection Angle End"].toDouble(),
			parameters["Distance Origin-Detector"].toDouble(),
			parameters["Distance Origin-Source"].toDouble());
		int volDim[3] = {
			parameters["Volume Dimension X"].toInt(),
			parameters["Volume Dimension Y"].toInt(),
			parameters["Volume Dimension Z"].toInt()
		};
		double volSpacing[3] = {
			parameters["Volume Spacing X"].toDouble(),
			parameters["Volume Spacing Y"].toDouble(),
			parameters["Volume Spacing Z"].toDouble()
		};
		dlg.fillVolumeGeometryValues(volDim, volSpacing);
		MdiChild* child = mainWnd->activeMdiChild();
		vtkSmartPointer<vtkImageData> img = child->getImageData();
		dlg.fillProjInputMapping(parameters["Detector Row Dimension"].toInt(),
			parameters["Detector Column Dimension"].toInt(), 
			parameters["Projection Angle Dimension"].toInt(),
			img->GetDimensions());
		dlg.fillAlgorithmValues(MapAlgoStringToIndex(parameters["Algorithm Type"].toString()), parameters["Number of Iterations"].toUInt());
		dlg.fillCorrectionValues(parameters["Center of Rotation Correction"].toBool(),
			parameters["Center of Rotation Offset"].toDouble());
	}
	if (dlg.exec() != QDialog::Accepted)
		return false;

	parameters["Projection Geometry"]      = dlg.ProjGeomType->currentText();
	parameters["Detector Spacing X"]       = dlg.ProjGeomDetectorSpacingX->value();
	parameters["Detector Spacing Y"]       = dlg.ProjGeomDetectorSpacingY->value();
	parameters["Projection Angle Start"]   = dlg.ProjGeomProjAngleStart->value();
	parameters["Projection Angle End"]     = dlg.ProjGeomProjAngleEnd->value();
	parameters["Distance Origin-Detector"] = dlg.ProjGeomDistOriginDetector->value();
	parameters["Distance Origin-Source"]   = dlg.ProjGeomDistOriginSource->value();
	if (filter->Name() == "ASTRA Forward Projection")
	{
		parameters["Detector Row Count"]       = dlg.ProjGeomDetectorPixelsY->value();
		parameters["Detector Column Count"]    = dlg.ProjGeomDetectorPixelsX->value();
		parameters["Projection Angle Count"]   = dlg.ProjGeomProjCount->value();
	}
	else    // Reconstruction:
	{
		int detRowDim = dlg.ProjInputDetectorRowDim->currentIndex();
		int detColDim = dlg.ProjInputDetectorColDim->currentIndex();
		int projAngleDim = dlg.ProjInputProjAngleDim->currentIndex();
		parameters["Volume Dimension X"] = dlg.VolGeomDimensionX->value();
		parameters["Volume Dimension Y"] = dlg.VolGeomDimensionY->value();
		parameters["Volume Dimension Z"] = dlg.VolGeomDimensionZ->value();
		parameters["Volume Spacing X"] = dlg.VolGeomSpacingX->value();
		parameters["Volume Spacing Y"] = dlg.VolGeomSpacingY->value();
		parameters["Volume Spacing Z"] = dlg.VolGeomSpacingZ->value();
		parameters["Algorithm Type"] = MapAlgoIndexToString(dlg.AlgorithmType->currentIndex());
		parameters["Number of Iterations"] = dlg.AlgorithmIterations->value();
		parameters["Center of Rotation Correction"] = dlg.CorrectionCenterOfRotation->isChecked();
		parameters["Center of Rotation Offset"] = dlg.CorrectionCenterOfRotationOffset->value();
		if ((detColDim % 3) == (detRowDim % 3) || (detColDim % 3) == (projAngleDim % 3) || (detRowDim % 3) == (projAngleDim % 3))
		{
			QMessageBox::warning(mainWnd, "ASTRA", "One of the axes (x, y, z) has been specified for more than one usage out of "
				"(detector row / detector column / projection angle) dimensions. "
				"Make sure each axis is used exactly for one dimension!");
			return false;
		}
		parameters["Detector Row Dimension"] = detRowDim;
		parameters["Detector Column Dimension"] = detColDim;
		parameters["Projection Angle Dimension"] = projAngleDim;

		MdiChild* child = mainWnd->activeMdiChild();
		vtkSmartPointer<vtkImageData> img = child->getImageData();
		int const * dim = img->GetDimensions();
		parameters["Detector Row Count"] = dim[detRowDim % 3];
		parameters["Detector Column Count"] = dim[detColDim % 3];
		parameters["Projection Angles Count"] = dim[projAngleDim % 3];
	}
	return true;
}