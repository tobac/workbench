
/*LICENSE_START*/
/* 
 *  Copyright 1995-2011 Washington University School of Medicine 
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

#define __SURFACE_TYPE_ENUM_DECLARE__
#include "SurfaceTypeEnum.h"
#undef __SURFACE_TYPE_ENUM_DECLARE__

using namespace caret;

/**
 * Constructor.
 *
 * @param e
 *    An enumerated value.
 * @param name
 *    Name of enumberated value.
 */
SurfaceTypeEnum::SurfaceTypeEnum(const Enum e,
                                 const AString& name,
                                 const AString& guiName,
                                 const AString& giftiName)
{
    this->e = e;
    this->integerCode = SurfaceTypeEnum::integerCodeGenerator++;
    this->name = name;
    this->guiName = guiName;
    this->giftiName = giftiName;
}

/**
 * Destructor.
 */
SurfaceTypeEnum::~SurfaceTypeEnum()
{
}

/**
 * Initialize the enumerated metadata.
 */
void
SurfaceTypeEnum::initialize()
{
    if (initializedFlag) {
        return;
    }
    initializedFlag = true;

    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_UNKNOWN, 
                                       "SURFACE_TYPE_UNKNOWN", 
                                       "Unknown",
                                       "Unknown"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_RECONSTRUCTION, 
                                       "SURFACE_TYPE_RECONSTRUCTION", 
                                       "Reconstruction",
                                       "Reconstruction"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_ANATOMICAL, 
                                       "SURFACE_TYPE_ANATOMICAL", 
                                       "Anatomical",
                                       "Anatomical"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_INFLATED, 
                                       "SURFACE_TYPE_INFLATED", 
                                       "Inflated",
                                       "Inflated"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_VERY_INFLATED, 
                                       "SURFACE_TYPE_VERY_INFLATED", 
                                       "VeryInflated",
                                       "VeryInflated"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_SPHERICAL, 
                                       "SURFACE_TYPE_SPHERICAL", 
                                       "Spherical",
                                       "Spherical"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_SEMI_SPHERICAL, 
                                       "SURFACE_TYPE_SEMI_SPHERICAL", 
                                       "SemiSpherical",
                                       "SemiSpherical"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_ELLIPSOID, 
                                       "SURFACE_TYPE_ELLIPSOID", 
                                       "Ellipsoid",
                                       "Ellipsoid"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_FLAT, 
                                       "SURFACE_TYPE_FLAT", 
                                       "Flat",
                                       "Flat"));
    enumData.push_back(SurfaceTypeEnum(SURFACE_TYPE_HULL, 
                                       "SURFACE_TYPE_HULL", 
                                       "Hull",
                                       "Hull"));    
}

/**
 * Find the data for and enumerated value.
 * @param e
 *     The enumerated value.
 * @return Pointer to data for this enumerated type
 * or NULL if no data for type or if type is invalid.
 */
const SurfaceTypeEnum*
SurfaceTypeEnum::findData(const Enum e)
{
    initialize();

    size_t num = enumData.size();
    for (size_t i = 0; i < num; i++) {
        const SurfaceTypeEnum* d = &enumData[i];
        if (d->e == e) {
            return d;
        }
    }

    return NULL;
}

/**
 * Get a string representation of the enumerated type.
 * @param e 
 *     Enumerated value.
 * @return 
 *     String representing enumerated value.
 */
AString 
SurfaceTypeEnum::toName(Enum e) {
    initialize();
    
    const SurfaceTypeEnum* st = findData(e);
    return st->name;
}

/**
 * Get an enumerated value corresponding to its name.
 * @param s 
 *     Name of enumerated value.
 * @param isValidOut 
 *     If not NULL, it is set indicating that a
 *     enum value exists for the input name.
 * @return 
 *     Enumerated value.
 */
SurfaceTypeEnum::Enum 
SurfaceTypeEnum::fromName(const AString& s, bool* isValidOut)
{
    initialize();
    
    bool validFlag = false;
    Enum e = SURFACE_TYPE_UNKNOWN;
    
    for (std::vector<SurfaceTypeEnum>::iterator iter = enumData.begin();
         iter != enumData.end();
         iter++) {
        const SurfaceTypeEnum& d = *iter;
        if (d.name == s) {
            e = d.e;
            validFlag = true;
            break;
        }
    }
    
    if (isValidOut != 0) {
        *isValidOut = validFlag;
    }
    return e;
}

/**
 * Get a GUI string representation of the enumerated type.
 * @param e 
 *     Enumerated value.
 * @return 
 *     String representing enumerated value.
 */
AString 
SurfaceTypeEnum::toGuiName(Enum e) {
    initialize();
    
    const SurfaceTypeEnum* st = findData(e);
    return st->guiName;
}

/**
 * Get an enumerated value corresponding to its GUI name.
 * @param s 
 *     Name of enumerated value.
 * @param isValidOut 
 *     If not NULL, it is set indicating that a
 *     enum value exists for the input name.
 * @return 
 *     Enumerated value.
 */
SurfaceTypeEnum::Enum 
SurfaceTypeEnum::fromGuiName(const AString& s, bool* isValidOut)
{
    initialize();
    
    bool validFlag = false;
    Enum e = SURFACE_TYPE_UNKNOWN;
    
    for (std::vector<SurfaceTypeEnum>::iterator iter = enumData.begin();
         iter != enumData.end();
         iter++) {
        const SurfaceTypeEnum& d = *iter;
        if (d.guiName == s) {
            e = d.e;
            validFlag = true;
            break;
        }
    }
    
    if (isValidOut != 0) {
        *isValidOut = validFlag;
    }
    return e;
}

/**
 * Get the integer code for a data type.
 *
 * @return
 *    Integer code for data type.
 */
int32_t
SurfaceTypeEnum::toIntegerCode(Enum e)
{
    initialize();
    const SurfaceTypeEnum* ndt = findData(e);
    return ndt->integerCode;
}

/**
 * Find the data type corresponding to an integer code.
 *
 * @param integerCode
 *     Integer code for enum.
 * @param isValidOut
 *     If not NULL, on exit isValidOut will indicate if
 *     integer code is valid.
 * @return
 *     Enum for integer code.
 */
SurfaceTypeEnum::Enum
SurfaceTypeEnum::fromIntegerCode(const int32_t integerCode, bool* isValidOut)
{
    initialize();
    
    bool validFlag = false;
    Enum e = SURFACE_TYPE_UNKNOWN;
    
    for (std::vector<SurfaceTypeEnum>::iterator iter = enumData.begin();
         iter != enumData.end();
         iter++) {
        const SurfaceTypeEnum& ndt = *iter;
        if (ndt.integerCode == integerCode) {
            e = ndt.e;
            validFlag = true;
            break;
        }
    }
    
    if (isValidOut != 0) {
        *isValidOut = validFlag;
    }
    return e;
}

/**
 * Get a GIFTI Name string representation of the enumerated type.
 * @param e 
 *     Enumerated value.
 * @return 
 *     String representing enumerated value.
 */
AString 
SurfaceTypeEnum::toGiftiName(Enum e) {
    initialize();
    
    const SurfaceTypeEnum* st = findData(e);
    return st->giftiName;
}

/**
 * Get an enumerated value corresponding to its GIFTI name.
 * @param s 
 *     Name of enumerated value.
 * @param isValidOut 
 *     If not NULL, it is set indicating that a
 *     enum value exists for the input name.
 * @return 
 *     Enumerated value.
 */
SurfaceTypeEnum::Enum 
SurfaceTypeEnum::fromGiftiName(const AString& s, bool* isValidOut)
{
    initialize();
    
    bool validFlag = false;
    Enum e = SURFACE_TYPE_UNKNOWN;
    
    for (std::vector<SurfaceTypeEnum>::iterator iter = enumData.begin();
         iter != enumData.end();
         iter++) {
        const SurfaceTypeEnum& d = *iter;
        if (d.giftiName == s) {
            e = d.e;
            validFlag = true;
            break;
        }
    }
    
    if (isValidOut != 0) {
        *isValidOut = validFlag;
    }
    return e;
}

/**
 * Get all of the enumerated type values.  The values can be used
 * as parameters to toXXX() methods to get associated metadata.
 *
 * @param allEnums
 *     A vector that is OUTPUT containing all of the enumerated values.
 */
void
SurfaceTypeEnum::getAllEnums(std::vector<SurfaceTypeEnum::Enum>& allEnums)
{
    if (initializedFlag == false) initialize();
    
    allEnums.clear();
    
    for (std::vector<SurfaceTypeEnum>::iterator iter = enumData.begin();
         iter != enumData.end();
         iter++) {
        allEnums.push_back(iter->e);
    }
}

