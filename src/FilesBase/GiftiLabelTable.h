#ifndef __GIFTILABELTABLE_H__
#define __GIFTILABELTABLE_H__

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


#include "CaretObject.h"
#include "TracksModificationInterface.h"

#include "GiftiException.h"

#include <map>
#include <set>
#include <stdint.h>

#include <AString.h>

namespace caret {

class GiftiLabel;
    
class XmlWriter;
class XmlException;
    
/**
 * Maintains a GIFTI Label Table using key/value pairs.
 */
class GiftiLabelTable : public CaretObject, TracksModificationInterface {

public:
    GiftiLabelTable();

    GiftiLabelTable(const GiftiLabelTable& glt);

    GiftiLabelTable& operator=(const GiftiLabelTable& glt);

    virtual ~GiftiLabelTable();

private:
    void copyHelper(const GiftiLabelTable& glt);

    void initializeMembersGiftiLabelTable();
    
public:
    void clear();

    std::map<int32_t,int32_t> append(const GiftiLabelTable& glt);

    int32_t addLabel(
                    const AString& labelName,
                    const float red,
                    const float green,
                    const float blue,
                    const float alpha);

    int32_t addLabel(
                    const AString& labelName,
                    const float red,
                    const float green,
                    const float blue);

    int32_t addLabel(
                    const AString& labelName,
                    const int32_t red,
                    const int32_t green,
                    const int32_t blue,
                    const int32_t alpha);

    int32_t addLabel(
                    const AString& labelName,
                    const int32_t red,
                    const int32_t green,
                    const int32_t blue);

    int32_t addLabel(const GiftiLabel* glt);

    void deleteLabel(const int32_t key);

    void deleteLabel(const GiftiLabel* label);

    void deleteUnusedLabels(const std::set<int32_t>& usedLabelKeys);

    void insertLabel(const GiftiLabel* label);

    int32_t getLabelKeyFromName(const AString& name) const;

    const GiftiLabel* getLabel(const AString& labelName) const;

    const GiftiLabel* getLabelBestMatching(const AString& name) const;

    const GiftiLabel* getLabel(const int32_t key) const;

    int32_t getUnassignedLabelKey() const;

    int32_t getNumberOfLabels() const;

    AString getLabelName(const int32_t key) const;

    void setLabelName(
                    const int32_t key,
                    const AString& name);

    void setLabel(const int32_t key,
                    const AString& name,
                    const float red,
                    const float green,
                    const float blue,
                    const float alpha);

    void setLabel(const int32_t key,
                  const AString& name,
                  const float red,
                  const float green,
                  const float blue,
                  const float alpha,
                  const float x,
                  const float y, 
                  const float z);
    
    bool isLabelSelected(const int32_t key) const;

    void setLabelSelected(
                    const int32_t key,
                    const bool sel);

    void setSelectionStatusForAllLabels(const bool newStatus);

    float getLabelAlpha(const int32_t key) const;

    void getLabelColor(const int32_t key, float rgbaOut[4]) const;

    void setLabelColor(
                    const int32_t key,
                    const float color[4]);

    std::vector<int32_t> getLabelKeysSortedByName() const;

    void resetLabelCounts();

    void createLabelsForKeys(const std::set<int32_t>& newKeys);

    void writeAsXML(XmlWriter& xmlWriter) throw (GiftiException);

    AString toString() const;

    AString toFormattedString(const AString& indentation);

    //void readFromXMLDOM(const Node* rootNode)
    //        throw (GiftiException);

    void readFromXmlString(const AString& s)
            throw (GiftiException);

    void setModified();

    void clearModified();

    bool isModified() const;

    //Iterator<int32_t> getKeysIterator() const;

    std::set<int32_t> getKeys() const;

private:
    /** The label table storage.  Use a TreeMap since label keys
 may be sparse.
*/
    typedef std::map<int32_t, GiftiLabel*> LABELS_MAP;
    typedef std::map<int32_t, GiftiLabel*>::iterator LABELS_MAP_ITERATOR;
    typedef std::map<int32_t, GiftiLabel*>::const_iterator LABELS_MAP_CONST_ITERATOR;

    LABELS_MAP labelsMap;

    int32_t generateUnusedKey() const;
    
    /**tracks modification status */
    bool modifiedFlag;

};

} // namespace

#endif // __GIFTILABELTABLE_H__
