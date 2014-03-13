
/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#define __CIFTI_CONNECTIVITY_MATRIX_PARCEL_FILE_DECLARE__
#include "CiftiConnectivityMatrixParcelFile.h"
#undef __CIFTI_CONNECTIVITY_MATRIX_PARCEL_FILE_DECLARE__

#include "CaretLogger.h"
#include "CiftiFacade.h"
#include "CiftiInterface.h"
#include "FastStatistics.h"
#include "NodeAndVoxelColoring.h"
#include "Palette.h"
#include "PaletteColorMapping.h"
#include "PaletteFile.h"
#include "SceneClass.h"
#include "SceneClassArray.h"

using namespace caret;


    
/**
 * \class caret::CiftiConnectivityMatrixParcelFile 
 * \brief Connectivity Parcel x Parcel File
 * \ingroup Files
 *
 * Contains connectivity matrix that measures connectivity from parcels
 * to parcels.
 */

/**
 * Constructor.
 */
CiftiConnectivityMatrixParcelFile::CiftiConnectivityMatrixParcelFile()
: CiftiMappableConnectivityMatrixDataFile(DataFileTypeEnum::CONNECTIVITY_PARCEL,
                                          CiftiMappableDataFile::FILE_READ_DATA_ALL,
                                          CIFTI_INDEX_TYPE_PARCELS,
                                          CIFTI_INDEX_TYPE_PARCELS,
                                          CiftiMappableDataFile::DATA_ACCESS_WITH_ROW_METHODS,
                                          CiftiMappableDataFile::DATA_ACCESS_INVALID)
/*,ChartableBrainordinateInterface()*/
{
    for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS; i++) {
        m_chartingEnabledForTab[i] = false;
    }
}

/**
 * Destructor.
 */
CiftiConnectivityMatrixParcelFile::~CiftiConnectivityMatrixParcelFile()
{
    
}

/**
 * Get the matrix RGBA coloring for this matrix data creator.
 *
 * @param numberOfRowsOut
 *    Number of rows in the coloring matrix.
 * @param numberOfColumnsOut
 *    Number of rows in the coloring matrix.
 * @param rgbaOut
 *    RGBA coloring output with number of elements
 *    (numberOfRowsOut * numberOfColumnsOut * 4).
 * @return
 *    True if data output data is valid, else false.
 */
bool
CiftiConnectivityMatrixParcelFile::getMatrixDataRGBA(int32_t& numberOfRowsOut,
                                                    int32_t& numberOfColumnsOut,
                                                    std::vector<float>& rgbaOut) const
{
    return helpLoadChartDataMatrixForMap(0,
                                         numberOfRowsOut,
                                         numberOfColumnsOut,
                                         rgbaOut);
}

/**
 * Get the value, row name, and column name for a cell in the matrix.
 *
 * @param rowIndex
 *     The row index.
 * @param columnIndex
 *     The column index.
 * @param cellValueOut
 *     Output containing value in the cell.
 * @param rowNameOut
 *     Name of row corresponding to row index.
 * @param columnNameOut
 *     Name of column corresponding to column index.
 * @return
 *     True if the output values are valid (valid row/column indices).
 */
bool
CiftiConnectivityMatrixParcelFile::getMatrixCellAttributes(const int32_t rowIndex,
                                                           const int32_t columnIndex,
                                                           float& cellValueOut,
                                                           AString& rowNameOut,
                                                           AString& columnNameOut) const
{
    if ((rowIndex >= 0)
        && (rowIndex < m_ciftiFacade->getNumberOfRows())
        && (columnIndex >= 0)
        && (columnIndex < m_ciftiFacade->getNumberOfColumns())) {
        const CiftiXML& xml = m_ciftiInterface->getCiftiXML();
        
        const std::vector<CiftiParcelsMap::Parcel>& rowsParcelsMap = xml.getParcelsMap(CiftiXML::ALONG_COLUMN).getParcels();
        CaretAssertVectorIndex(rowsParcelsMap, rowIndex);
        rowNameOut = rowsParcelsMap[rowIndex].m_name;
        
        const std::vector<CiftiParcelsMap::Parcel>& columnsParcelsMap = xml.getParcelsMap(CiftiXML::ALONG_ROW).getParcels();
        CaretAssertVectorIndex(columnsParcelsMap, columnIndex);
        columnNameOut = columnsParcelsMap[columnIndex].m_name;
        
        const int32_t numberOfElementsInRow = m_ciftiInterface->getNumberOfColumns();
        std::vector<float> rowData(numberOfElementsInRow);
        m_ciftiInterface->getRow(&rowData[0],
                                 rowIndex);
        CaretAssertVectorIndex(rowData, columnIndex);
        cellValueOut = rowData[columnIndex];
        
        return true;
    }
    
    return false;
}

/**
 * @return Is charting enabled for this file?
 */
bool
CiftiConnectivityMatrixParcelFile::isChartingEnabled(const int32_t tabIndex) const
{
    CaretAssertArrayIndex(m_chartingEnabledForTab,
                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                          tabIndex);
    return m_chartingEnabledForTab[tabIndex];
}

/**
 * @return Return true if the file's current state supports
 * charting data, else false.  Typically a brainordinate file
 * is chartable if it contains more than one map.
 */
bool
CiftiConnectivityMatrixParcelFile::isChartingSupported() const
{
    return true;    
}

/**
 * Set charting enabled for this file.
 *
 * @param enabled
 *    New status for charting enabled.
 */
void
CiftiConnectivityMatrixParcelFile::setChartingEnabled(const int32_t tabIndex,
                                                      const bool enabled)
{
    CaretAssertArrayIndex(m_chartingEnabledForTab,
                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                          tabIndex);
    m_chartingEnabledForTab[tabIndex] = enabled;
}

/**
 * Get chart data types supported by the file.
 *
 * @param chartDataTypesOut
 *    Chart types supported by this file.
 */
void
CiftiConnectivityMatrixParcelFile::getSupportedChartDataTypes(std::vector<ChartDataTypeEnum::Enum>& chartDataTypesOut) const
{
    chartDataTypesOut.clear();
    chartDataTypesOut.push_back(ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX);
}

/**
 * @return The CaretMappableDataFile that implements this interface.
 */
CaretMappableDataFile*
CiftiConnectivityMatrixParcelFile::getCaretMappableDataFile()
{
    return dynamic_cast<CaretMappableDataFile*>(this);
}

/**
 * @return The CaretMappableDataFile that implements this interface.
 */
const CaretMappableDataFile*
CiftiConnectivityMatrixParcelFile::getCaretMappableDataFile() const
{
    return dynamic_cast<const CaretMappableDataFile*>(this);
}

/**
 * Save file data from the scene.  For subclasses that need to
 * save to a scene, this method should be overriden.  sceneClass
 * will be valid and any scene data should be added to it.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass to which data members should be added.
 */
void
CiftiConnectivityMatrixParcelFile::saveFileDataToScene(const SceneAttributes* sceneAttributes,
                                                  SceneClass* sceneClass)
{
    CiftiMappableDataFile::saveFileDataToScene(sceneAttributes,
                                               sceneClass);
    
    sceneClass->addBooleanArray("m_chartingEnabledForTab",
                                m_chartingEnabledForTab,
                                BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS);
}

/**
 * Restore file data from the scene.  For subclasses that need to
 * restore from a scene, this method should be overridden. The scene class
 * will be valid and any scene data may be obtained from it.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass for the instance of a class that implements
 *     this interface.  Will NEVER be NULL.
 */
void
CiftiConnectivityMatrixParcelFile::restoreFileDataFromScene(const SceneAttributes* sceneAttributes,
                                                       const SceneClass* sceneClass)
{
    CiftiMappableDataFile::restoreFileDataFromScene(sceneAttributes,
                                                    sceneClass);
    
    const ScenePrimitiveArray* tabArray = sceneClass->getPrimitiveArray("m_chartingEnabledForTab");
    if (tabArray != NULL) {
        sceneClass->getBooleanArrayValue("m_chartingEnabledForTab",
                                         m_chartingEnabledForTab,
                                         BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS);
    }
    else {
        /*
         * Obsolete value when charting was not 'per tab'
         */
        const bool chartingEnabled = sceneClass->getBooleanValue("m_chartingEnabled",
                                                                 false);
        for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS; i++) {
            m_chartingEnabledForTab[i] = chartingEnabled;
        }
    }
}

