/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "AlgorithmMetricExtrema.h"
#include "AlgorithmException.h"
#include "AlgorithmMetricSmoothing.h"
#include "CaretHeap.h"
#include "CaretOMP.h"
#include "GeodesicHelper.h"
#include "MetricFile.h"
#include "SurfaceFile.h"
#include "TopologyHelper.h"
#include <cmath>
#include <utility>
#include <vector>
#include <iostream>

using namespace caret;
using namespace std;

AString AlgorithmMetricExtrema::getCommandSwitch()
{
    return "-metric-extrema";
}

AString AlgorithmMetricExtrema::getShortDescription()
{
    return "FIND THE EXTREMA IN A METRIC FILE";
}

OperationParameters* AlgorithmMetricExtrema::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    ret->addSurfaceParameter(1, "surface", "the surface to use for distance information");
    
    ret->addMetricParameter(2, "metric-in", "the metric to fine the extrema of");
    
    ret->addDoubleParameter(3, "distance", "the minimum distance between identified extrema of the same type");
    
    ret->addMetricOutputParameter(4, "metric-out", "the output extrema metric");
    
    OptionalParameter* presmoothOpt = ret->createOptionalParameter(5, "-presmooth", "smooth the metric before finding extrema");
    presmoothOpt->addDoubleParameter(1, "presmoothing", "the sigma for the gaussian smoothing kernel, in mm");
    
    OptionalParameter* thresholdOpt = ret->createOptionalParameter(6, "-threshold", "ignore small extrema");
    thresholdOpt->addDoubleParameter(1, "low", "the largest value to consider for being a minimum");
    thresholdOpt->addDoubleParameter(2, "high", "the smallest value to consider for being a maximum");
    
    ret->createOptionalParameter(7, "-sum-columns", "output the sum of the extrema columns instead of each column separately");
    
    ret->createOptionalParameter(8, "-consolidate-mode", "use consolidation of local minima instead of a large neighborhood");
    
    OptionalParameter* columnSelect = ret->createOptionalParameter(9, "-column", "select a single column to find extrema in");
    columnSelect->addStringParameter(1, "column", "the column number or name");
    
    ret->setHelpText(
        AString("Finds extrema in a metric file, such that no two extrema of the same type are within <distance> of each other.  ") +
        "The extrema are labeled as -1 for minima, 1 for maxima, 0 otherwise.\n\n" +
        "If -sum-columns is specified, these extrema colums are summed across the input maps, and the output has a single column with this result.\n\n" +
        "By default, a datapoint is an extrema only if it is more extreme than every other datapoint that is within <distance> from it.  " +
        "If -consolidate-mode is used, it instead starts by finding all datapoints that are more extreme than the immediate neighbors, " +
        "then while there are any extrema within <distance> of each other, take the two extrema closest to each other and merge them into one by a weighted average " +
        "based on how many original extrema have been merged into each.\n\n" +
        "By default, all input columns are used with no smoothing, use -column to specify a single column to use, and -presmooth to smooth the input before " +
        "finding the extrema."
    );
    return ret;
}

void AlgorithmMetricExtrema::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    SurfaceFile* mySurf = myParams->getSurface(1);
    MetricFile* myMetric = myParams->getMetric(2);
    float distance = (float)myParams->getDouble(3);
    MetricFile* myMetricOut = myParams->getOutputMetric(4);
    OptionalParameter* presmoothOpt = myParams->getOptionalParameter(5);
    float presmooth = -1.0f;
    if (presmoothOpt->m_present)
    {
        presmooth = (float)presmoothOpt->getDouble(1);
        if (presmooth <= 0.0f)
        {
            throw AlgorithmException("smoothing amount must be positive");
        }
    }
    bool thresholdMode = false;
    float lowThresh = 0.0f, highThresh = 0.0f;
    OptionalParameter* thresholdOpt = myParams->getOptionalParameter(6);
    if (thresholdOpt->m_present)
    {
        thresholdMode = true;
        lowThresh = (float)thresholdOpt->getDouble(1);
        highThresh = (float)thresholdOpt->getDouble(2);
    }
    bool sumColumns = myParams->getOptionalParameter(7)->m_present;
    bool consolidateMode = myParams->getOptionalParameter(8)->m_present;
    OptionalParameter* columnSelect = myParams->getOptionalParameter(9);
    int columnNum = -1;
    if (columnSelect->m_present)
    {//set up to use the single column
        columnNum = (int)myMetric->getMapIndexFromNameOrNumber(columnSelect->getString(1));
        if (columnNum < 0)
        {
            throw AlgorithmException("invalid column specified");
        }
    }
    if (thresholdMode)
    {
        AlgorithmMetricExtrema(myProgObj, mySurf, myMetric, distance, myMetricOut, lowThresh, highThresh, presmooth, sumColumns, consolidateMode, columnNum);
    } else {
        AlgorithmMetricExtrema(myProgObj, mySurf, myMetric, distance, myMetricOut, presmooth, sumColumns, consolidateMode, columnNum);
    }
}

AlgorithmMetricExtrema::AlgorithmMetricExtrema(ProgressObject* myProgObj, const SurfaceFile* mySurf,const MetricFile* myMetric, const float& distance,
                                               MetricFile* myMetricOut, const float& presmooth, const bool& sumColumns, const bool& consolidateMode, const int& columnNum) : AbstractAlgorithm(myProgObj)
{
    LevelProgress myProgress(myProgObj);
    int numNodes = mySurf->getNumberOfNodes();
    if (myMetric->getNumberOfNodes() != numNodes)
    {
        throw AlgorithmException("input metric has different number of vertices than input surface");
    }
    int numCols = myMetric->getNumberOfColumns();
    if (columnNum < -1 || columnNum >= numCols)
    {
        throw AlgorithmException("invalid column number");
    }
    int useCol = columnNum;
    const MetricFile* toProcess = myMetric;
    MetricFile tempMetric;
    if (presmooth > 0.0f)
    {
        AlgorithmMetricSmoothing(NULL, mySurf, myMetric, presmooth, &tempMetric, NULL, false, columnNum);
        toProcess = &tempMetric;
        if (columnNum != -1)
        {
            useCol = 0;
        }
    }
    vector<vector<int32_t> > neighborhoods;
    if (!consolidateMode)
    {
        precomputeNeighborhoods(mySurf, distance, neighborhoods);
    }
    if (columnNum == -1)
    {
        vector<float> scratchcol(numNodes, 0.0f);
        vector<int> minima, maxima;
        if (sumColumns)
        {
            myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        } else {
            myMetricOut->setNumberOfNodesAndColumns(numNodes, numCols);
        }
        myMetricOut->setStructure(mySurf->getStructure());
        for (int i = 0; i < numCols; ++i)
        {
            if (consolidateMode)
            {
                findMinimaConsolidate(mySurf, toProcess->getValuePointerForColumn(i), distance, minima, maxima);
            } else {
                findMinimaNeighborhoods(toProcess->getValuePointerForColumn(i), neighborhoods, minima, maxima);
            }
            if (sumColumns)
            {
                int numelems = (int)minima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[minima[j]] -= 1.0f;
                }
                numelems = (int)maxima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[maxima[j]] += 1.0f;
                }
            } else {
                int numelems = (int)minima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[minima[j]] = -1.0f;
                }
                numelems = (int)maxima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[maxima[j]] = 1.0f;
                }
                myMetricOut->setValuesForColumn(i, scratchcol.data());
                numelems = (int)minima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[minima[j]] = 0.0f;
                }
                numelems = (int)maxima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[maxima[j]] = 0.0f;
                }
            }
        }
        if (sumColumns)
        {
            myMetricOut->setValuesForColumn(0, scratchcol.data());
        }
    } else {
        vector<float> scratchcol(numNodes, 0.0f);
        vector<int> minima, maxima;
        myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        if (consolidateMode)
        {
            findMinimaConsolidate(mySurf, toProcess->getValuePointerForColumn(useCol), distance, minima, maxima);
        } else {
            findMinimaNeighborhoods(toProcess->getValuePointerForColumn(useCol), neighborhoods, minima, maxima);
        }
        int numelems = (int)minima.size();
        for (int j = 0; j < numelems; ++j)
        {
            scratchcol[minima[j]] = -1.0f;
        }
        numelems = (int)maxima.size();
        for (int j = 0; j < numelems; ++j)
        {
            scratchcol[maxima[j]] = 1.0f;
        }
        myMetricOut->setValuesForColumn(0, scratchcol.data());
    }
}

AlgorithmMetricExtrema::AlgorithmMetricExtrema(ProgressObject* myProgObj, const SurfaceFile* mySurf,const MetricFile* myMetric, const float& distance,
                                               MetricFile* myMetricOut, const float& lowThresh, const float& highThresh, const float& presmooth, const bool& sumColumns, const bool& consolidateMode, const int& columnNum) : AbstractAlgorithm(myProgObj)
{
    LevelProgress myProgress(myProgObj);
    int numNodes = mySurf->getNumberOfNodes();
    if (myMetric->getNumberOfNodes() != numNodes)
    {
        throw AlgorithmException("input metric has different number of vertices than input surface");
    }
    int numCols = myMetric->getNumberOfColumns();
    if (columnNum < -1 || columnNum >= numCols)
    {
        throw AlgorithmException("invalid column number");
    }
    int useCol = columnNum;
    const MetricFile* toProcess = myMetric;
    MetricFile tempMetric;
    if (presmooth > 0.0f)
    {
        AlgorithmMetricSmoothing(NULL, mySurf, myMetric, presmooth, &tempMetric, NULL, false, columnNum);
        toProcess = &tempMetric;
        if (columnNum != -1)
        {
            useCol = 0;
        }
    }
    vector<vector<int32_t> > neighborhoods;
    if (!consolidateMode)
    {
        precomputeNeighborhoods(mySurf, distance, neighborhoods);
    }
    if (columnNum == -1)
    {
        vector<float> scratchcol(numNodes, 0.0f);
        vector<int> minima, maxima;
        if (sumColumns)
        {
            myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        } else {
            myMetricOut->setNumberOfNodesAndColumns(numNodes, numCols);
        }
        for (int i = 0; i < numCols; ++i)
        {
            if (consolidateMode)
            {
                findMinimaConsolidate(mySurf, toProcess->getValuePointerForColumn(i), distance, lowThresh, highThresh, minima, maxima);
            } else {
                findMinimaNeighborhoods(toProcess->getValuePointerForColumn(i), neighborhoods, lowThresh, highThresh, minima, maxima);
            }
            if (sumColumns)
            {
                int numelems = (int)minima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[minima[j]] -= 1.0f;
                }
                numelems = (int)maxima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[maxima[j]] += 1.0f;
                }
            } else {
                int numelems = (int)minima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[minima[j]] = -1.0f;
                }
                numelems = (int)maxima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[maxima[j]] = 1.0f;
                }
                myMetricOut->setValuesForColumn(i, scratchcol.data());
                numelems = (int)minima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[minima[j]] = 0.0f;
                }
                numelems = (int)maxima.size();
                for (int j = 0; j < numelems; ++j)
                {
                    scratchcol[maxima[j]] = 0.0f;
                }
            }
        }
        if (sumColumns)
        {
            myMetricOut->setValuesForColumn(0, scratchcol.data());
        }
    } else {
        vector<float> scratchcol(numNodes, 0.0f);
        vector<int> minima, maxima;
        myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        if (consolidateMode)
        {
            findMinimaConsolidate(mySurf, toProcess->getValuePointerForColumn(useCol), distance, minima, maxima);
        } else {
            findMinimaNeighborhoods(toProcess->getValuePointerForColumn(useCol), neighborhoods, minima, maxima);
        }
        int numelems = (int)minima.size();
        for (int j = 0; j < numelems; ++j)
        {
            scratchcol[minima[j]] = -1.0f;
        }
        numelems = (int)maxima.size();
        for (int j = 0; j < numelems; ++j)
        {
            scratchcol[maxima[j]] = 1.0f;
        }
        myMetricOut->setValuesForColumn(0, scratchcol.data());
    }
}

void AlgorithmMetricExtrema::precomputeNeighborhoods(const SurfaceFile* mySurf, const float& distance, vector<vector<int32_t> >& neighborhoods)
{
    int numNodes = mySurf->getNumberOfNodes();
    neighborhoods.resize(numNodes);
    CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();//can share this, we will only use 1-hop neighbors
    CaretPointer<GeodesicHelper> myGeoHelp = mySurf->getGeodesicHelper();//must be thread-private
    vector<float> junk;//also thread-private
    for (int i = 0; i < numNodes; ++i)
    {
        myGeoHelp->getNodesToGeoDist(i, distance, neighborhoods[i], junk);
        if (junk.size() < 7)
        {
            neighborhoods[i] = myTopoHelp->getNodeNeighbors(i);
        } else {
            int numelems = (int)junk.size();
            for (int j = 0; j < numelems; ++j)
            {
                if (neighborhoods[i][j] == i)
                {
                    neighborhoods[i].erase(neighborhoods[i].begin() + j);//we don't want the node itself, so erase it
                    break;
                }
            }
        }
    }
}

void AlgorithmMetricExtrema::findMinimaNeighborhoods(const float* data, const vector<vector<int32_t> >& neighborhoods, vector<int>& minima, vector<int>& maxima)
{
    int numNodes = (int)neighborhoods.size();
    minima.clear();
    maxima.clear();
    vector<int> minPos(numNodes, 1), maxPos(numNodes, 1);//mark off things that fail a comparison to reduce the work - these are just used as booleans, but we don't want bitwise packing slowing us down or dropping writes
    for (int i = 0; i < numNodes; ++i)
    {
        bool canBeMin = minPos[i], canBeMax = maxPos[i];
        if (canBeMin || canBeMax)
        {
            const vector<int32_t>& myneighbors = neighborhoods[i];
            int numNeigh = (int)myneighbors.size();
            if (numNeigh == 0) break;//don't count isolated nodes as minima or maxima
            float myval = data[i];
            int j = 0;
            if (canBeMin && canBeMax)//avoid the double-test unless both options are on the table
            {//should be fairly rare, and doesn't need to loop
                int32_t neighNode = myneighbors[0];//NOTE: the equals case should set one of these to false, so that only one of the two loops needs to execute
                float otherval = data[neighNode];
                if (myval < otherval)
                {
                    minPos[neighNode] = 0;
                } else {
                    canBeMin = false;//center being equal or greater means it is not a minimum, so stop testing that
                }
                if (myval > otherval)
                {
                    maxPos[neighNode] = 0;
                } else {
                    canBeMax = false;
                }
                j = 1;//don't test 0 again if we did the double test
            }
            if (canBeMax)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval > otherval)
                    {
                        maxPos[neighNode] = 0;//TODO: test if performing an intelligent comparison here is faster than doing unneeded stores
                    } else {
                        canBeMax = false;
                        break;
                    }
                }
            }
            if (canBeMin)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval < otherval)
                    {
                        minPos[neighNode] = 0;//ditto
                    } else {
                        canBeMin = false;
                        break;
                    }
                }
            }
            if (canBeMax)
            {
#pragma omp critical
                {
                    maxima.push_back(i);
                }
            }
            if (canBeMin)
            {
#pragma omp critical
                {
                    minima.push_back(i);
                }
            }
        }
    }
}

void AlgorithmMetricExtrema::findMinimaNeighborhoods(const float* data, const vector<vector<int32_t> >& neighborhoods, const float& lowThresh, const float& highThresh, vector<int>& minima, vector<int>& maxima)
{
    int numNodes = (int)neighborhoods.size();
    minima.clear();
    maxima.clear();
    vector<int> minPos(numNodes, 1), maxPos(numNodes, 1);//mark off things that fail a comparison to reduce the work - these are just used as booleans, but we don't want bitwise packing slowing us down or dropping parallel writes
    for (int i = 0; i < numNodes; ++i)
    {
        bool canBeMin = minPos[i], canBeMax = maxPos[i];
        if (canBeMin || canBeMax)
        {
            const vector<int32_t>& myneighbors = neighborhoods[i];
            int numNeigh = (int)myneighbors.size();
            if (numNeigh == 0) break;//don't count isolated nodes as minima or maxima
            float myval = data[i];
            if (myval >= lowThresh) canBeMin = false;//check thresholds
            if (myval <= highThresh) canBeMax = false;
            int j = 0;
            if (canBeMin && canBeMax)//avoid the double-test unless both options are on the table
            {//should be fairly rare, and doesn't need to loop
                int32_t neighNode = myneighbors[0];//NOTE: the equals case should set one of these to false, so that only one of the two loops needs to execute
                float otherval = data[neighNode];
                if (myval < otherval)
                {
                    minPos[neighNode] = 0;
                } else {
                    canBeMin = false;//center being equal or greater means it is not a minimum, so stop testing that
                }
                if (myval > otherval)
                {
                    maxPos[neighNode] = 0;
                } else {
                    canBeMax = false;
                }
                j = 1;//don't test 0 again if we did the double test
            }
            if (canBeMax)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval > otherval)
                    {
                        maxPos[neighNode] = 0;//TODO: test if performing an intelligent comparison here is faster than doing unneeded stores
                    } else {
                        canBeMax = false;
                        break;
                    }
                }
            }
            if (canBeMin)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval < otherval)
                    {
                        minPos[neighNode] = 0;//ditto
                    } else {
                        canBeMin = false;
                        break;
                    }
                }
            }
            if (canBeMax)
            {
#pragma omp critical
                {
                    maxima.push_back(i);
                }
            }
            if (canBeMin)
            {
#pragma omp critical
                {
                    minima.push_back(i);
                }
            }
        }
    }
}

void AlgorithmMetricExtrema::findMinimaConsolidate(const SurfaceFile* mySurf, const float* data, const float& distance, vector<int>& minima, vector<int>& maxima)
{
    int numNodes = mySurf->getNumberOfNodes();
    minima.clear();
    maxima.clear();
    vector<pair<int, int> > tempExtrema[2];
    vector<int> minPos(numNodes, 1), maxPos(numNodes, 1);//mark off things that fail a comparison to reduce the work - these are just used as booleans, but we don't want bitwise packing slowing us down or dropping writes
    CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();
    for (int i = 0; i < numNodes; ++i)
    {
        bool canBeMin = minPos[i], canBeMax = maxPos[i];
        if (canBeMin || canBeMax)
        {
            const vector<int32_t>& myneighbors = myTopoHelp->getNodeNeighbors(i);
            int numNeigh = (int)myneighbors.size();
            if (numNeigh == 0) continue;//don't count isolated nodes as minima or maxima
            float myval = data[i];
            int j = 0;
            if (canBeMin && canBeMax)//avoid the double-test unless both options are on the table
            {//should be fairly rare, and doesn't need to loop
                int32_t neighNode = myneighbors[0];//NOTE: the equals case should set one of these to false, so that only one of the two loops needs to execute
                float otherval = data[neighNode];
                if (myval < otherval)
                {
                    minPos[neighNode] = 0;
                } else {
                    canBeMin = false;//center being equal or greater means it is not a minimum, so stop testing that
                }
                if (myval > otherval)
                {
                    maxPos[neighNode] = 0;
                } else {
                    canBeMax = false;
                }
                j = 1;//don't test 0 again if we did the double test
            }
            if (canBeMax)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval > otherval)
                    {
                        maxPos[neighNode] = 0;//TODO: test if performing an intelligent comparison here is faster than doing unneeded stores
                    } else {
                        canBeMax = false;
                        break;
                    }
                }
            }
            if (canBeMin)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval < otherval)
                    {
                        minPos[neighNode] = 0;//ditto
                    } else {
                        canBeMin = false;
                        break;
                    }
                }
            }
            if (canBeMax)
            {
#pragma omp critical
                {
                    tempExtrema[0].push_back(pair<int, int>(i, 1));
                }
            }
            if (canBeMin)
            {
#pragma omp critical
                {
                    tempExtrema[1].push_back(pair<int, int>(i, 1));
                }
            }
        }
    }
    consolidateStep(mySurf, distance, tempExtrema, minima, maxima);
}

void AlgorithmMetricExtrema::findMinimaConsolidate(const SurfaceFile* mySurf, const float* data, const float& lowThresh, const float& highThresh, const float& distance, vector<int>& minima, vector<int>& maxima)
{
    int numNodes = mySurf->getNumberOfNodes();
    minima.clear();
    maxima.clear();
    vector<pair<int, int> > tempExtrema[2];
    vector<int> minPos(numNodes, 1), maxPos(numNodes, 1);//mark off things that fail a comparison to reduce the work - these are just used as booleans, but we don't want bitwise packing slowing us down or dropping writes
    CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();
    for (int i = 0; i < numNodes; ++i)
    {
        bool canBeMin = minPos[i], canBeMax = maxPos[i];
        if (canBeMin || canBeMax)
        {
            const vector<int32_t>& myneighbors = myTopoHelp->getNodeNeighbors(i);
            int numNeigh = (int)myneighbors.size();
            if (numNeigh == 0) continue;//don't count isolated nodes as minima or maxima
            float myval = data[i];
            if (myval >= lowThresh) canBeMin = false;//check thresholds
            if (myval <= highThresh) canBeMax = false;
            int j = 0;
            if (canBeMin && canBeMax)//avoid the double-test unless both options are on the table
            {//should be fairly rare, and doesn't need to loop
                int32_t neighNode = myneighbors[0];//NOTE: the equals case should set one of these to false, so that only one of the two loops needs to execute
                float otherval = data[neighNode];
                if (myval < otherval)
                {
                    minPos[neighNode] = 0;
                } else {
                    canBeMin = false;//center being equal or greater means it is not a minimum, so stop testing that
                }
                if (myval > otherval)
                {
                    maxPos[neighNode] = 0;
                } else {
                    canBeMax = false;
                }
                j = 1;//don't test 0 again if we did the double test
            }
            if (canBeMax)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval > otherval)
                    {
                        maxPos[neighNode] = 0;//TODO: test if performing an intelligent comparison here is faster than doing unneeded stores
                    } else {
                        canBeMax = false;
                        break;
                    }
                }
            }
            if (canBeMin)
            {
                for (; j < numNeigh; ++j)
                {
                    int32_t neighNode = myneighbors[j];
                    float otherval = data[neighNode];
                    if (myval < otherval)
                    {
                        minPos[neighNode] = 0;//ditto
                    } else {
                        canBeMin = false;
                        break;
                    }
                }
            }
            if (canBeMax)
            {
#pragma omp critical
                {
                    tempExtrema[0].push_back(pair<int, int>(i, 1));
                }
            }
            if (canBeMin)
            {
#pragma omp critical
                {
                    tempExtrema[1].push_back(pair<int, int>(i, 1));
                }
            }
        }
    }
    consolidateStep(mySurf, distance, tempExtrema, minima, maxima);
}

void AlgorithmMetricExtrema::consolidateStep(const SurfaceFile* mySurf, const float& distance, vector<pair<int, int> > initExtrema[2], vector<int>& minima, vector<int>& maxima)
{
    int numNodes = mySurf->getNumberOfNodes();
    vector<float> scratchDist(numNodes, -1.0f);
    for (int sign = 0; sign < 2; ++sign)
    {
        cout << "sign start" << endl;
        int numInitExtrema = (int)initExtrema[sign].size();
        vector<bool> removed(numInitExtrema, false);//track which extrema locations are dropped during consolidation - the one that isn't dropped in a merge has its node number changed
        vector<vector<float> > distmatrix(numInitExtrema, vector<float>(numInitExtrema, -1.0f));
        vector<vector<int64_t> > heapIDmatrix(numInitExtrema, vector<int64_t>(numInitExtrema, -1));
        CaretMinHeap<pair<int, int>, float> myDistHeap;
        vector<float> dists;
        vector<int32_t> neighbors;
        CaretPointer<GeodesicHelper> myGeoHelp = mySurf->getGeodesicHelper();
        for (int i = 0; i < numInitExtrema - 1; ++i)
        {
            myGeoHelp->getNodesToGeoDist(initExtrema[sign][i].first, distance, neighbors, dists);//use smooth distance to get whether they are close enough
            int numInDist = (int)dists.size();
            for (int j = 0; j < numInDist; ++j)
            {
                scratchDist[neighbors[j]] = dists[j];
            }
            for (int j = i + 1; j < numInitExtrema; ++j)
            {
                float tempf = scratchDist[initExtrema[sign][j].first];
                if (tempf > -0.5f)
                {
                    distmatrix[i][j] = tempf;
                    distmatrix[j][i] = tempf;
                    cout << "push (" << i << ", " << j << "), " << tempf << "...";
                    cout.flush();
                    int64_t tempID = myDistHeap.push(pair<int, int>(i, j), tempf);
                    cout << tempID << endl;
                    heapIDmatrix[i][j] = tempID;
                    heapIDmatrix[j][i] = tempID;
                }
            }
            for (int j = 0; j < numInDist; ++j)
            {
                scratchDist[neighbors[j]] = -1.0f;
            }
        }//initial distance matrix computed, now we iterate
        while (!myDistHeap.isEmpty())
        {
            cout << "pop...";
            cout.flush();
            pair<int, int> toMerge = myDistHeap.pop();//we don't need to know the key
            cout << "(" << toMerge.first << ", " << toMerge.second << ")" << endl;
            int extr1 = toMerge.first;
            int extr2 = toMerge.second;
            heapIDmatrix[extr1][extr2] = -1;
            heapIDmatrix[extr2][extr1] = -1;
            distmatrix[extr1][extr2] = -1.0f;
            distmatrix[extr2][extr1] = -1.0f;
            int weight1 = initExtrema[sign][extr1].second;
            int weight2 = initExtrema[sign][extr2].second;
            if (weight2 > weight1)//swap so weight1 is always bigger
            {
                int temp = weight2;
                weight2 = weight1;
                weight1 = temp;
                temp = extr2;
                extr2 = extr1;
                extr1 = temp;
            }
            int node1 = initExtrema[sign][extr1].first;
            int node2 = initExtrema[sign][extr2].first;
            removed[extr2] = true;//drop the one that has less weight, and modify the one that has more weight
            for (int j = 0; j < numInitExtrema; ++j)
            {
                if (!removed[j])
                {
                    int64_t tempID = heapIDmatrix[extr2][j];
                    if (tempID != -1)
                    {
                        cout << "remove " << tempID << endl;
                        myDistHeap.remove(tempID);
                        heapIDmatrix[extr2][j] = -1;
                        heapIDmatrix[j][extr2] = -1;
                    }
                }
            }
            vector<int32_t> pathnodes;
            vector<float> pathdists;
            myGeoHelp->getPathToNode(node1, node2, pathnodes, pathdists, false);
            if (pathdists.size() != 0)
            {
                float distToWalk = (pathdists.back() * weight2) / (weight1 + weight2);
                int walk = 1, maxSteps = (int)pathnodes.size();
                float prevdiff = abs(distToWalk - pathdists[0]);
                while (walk < maxSteps)
                {
                    float newdiff = abs(distToWalk - pathdists[walk]);
                    if (newdiff > prevdiff) break;
                    ++walk;
                }
                int newnode = pathnodes[walk - 1];
                initExtrema[sign][extr1].first = newnode;
                initExtrema[sign][extr1].second += weight2;//add the weights together
                myGeoHelp->getNodesToGeoDist(newnode, distance, neighbors, dists);
                int numInDist = (int)dists.size();
                for (int j = 0; j < numInDist; ++j)
                {
                    scratchDist[neighbors[j]] = dists[j];
                }
                for (int j = 0; j < numInitExtrema; ++j)
                {
                    if (!removed[j])
                    {
                        float tempf = scratchDist[initExtrema[sign][j].first];
                        distmatrix[extr1][j] = tempf;
                        distmatrix[j][extr1] = tempf;
                        int64_t tempID = heapIDmatrix[extr1][j];
                        if (tempf > -0.5f)
                        {
                            if (tempID != -1)
                            {
                                cout << "changekey " << tempID << ", " << tempf << endl;
                                myDistHeap.changekey(tempID, tempf);
                            } else {
                                cout << "push (" << extr1 << ", " << j << "), " << tempf << "...";
                                cout.flush();
                                tempID = myDistHeap.push(pair<int, int>(extr1, j), tempf);
                                cout << tempID << endl;
                                heapIDmatrix[extr1][j] = tempID;
                                heapIDmatrix[j][extr1] = tempID;
                            }
                        } else {
                            if (tempID != -1)
                            {
                                cout << "remove " << tempID << endl;
                                myDistHeap.remove(tempID);
                                heapIDmatrix[extr1][j] = -1;
                                heapIDmatrix[j][extr1] = -1;
                            }
                        }
                    }
                }
                for (int j = 0; j < numInDist; ++j)
                {
                    scratchDist[neighbors[j]] = -1.0f;
                }
            }
        }
        if (sign == 0)
        {
            for (int i = 0; i < numInitExtrema; ++i)
            {
                if (!removed[i])
                {
                    maxima.push_back(initExtrema[sign][i].first);
                }
            }
        } else {
            for (int i = 0; i < numInitExtrema; ++i)
            {
                if (!removed[i])
                {
                    minima.push_back(initExtrema[sign][i].first);
                }
            }
        }
    }
}

float AlgorithmMetricExtrema::getAlgorithmInternalWeight()
{
    return 1.0f;//override this if needed, if the progress bar isn't smooth
}

float AlgorithmMetricExtrema::getSubAlgorithmWeight()
{
    //return AlgorithmInsertNameHere::getAlgorithmWeight();//if you use a subalgorithm
    return 0.0f;
}
