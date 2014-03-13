#ifndef __DATA_FILE_CONTENT_INFORMATION_H__
#define __DATA_FILE_CONTENT_INFORMATION_H__

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


#include "CaretObject.h"



namespace caret {

    class DataFileContentInformation : public CaretObject {
        
    public:
        DataFileContentInformation();
        
        virtual ~DataFileContentInformation();
        
        void addNameAndValue(const AString& name,
                             const AString& value);
        
        void addNameAndValue(const AString& name,
                             const int32_t value);
        
        void addNameAndValue(const AString& name,
                             const int64_t value);
        
        void addNameAndValue(const AString& name,
                             const double value,
                             const int32_t precision = 3);
        
        void addNameAndValue(const AString& name,
                             const bool value);
        
        void addText(const AString& text);
        
        AString getInformationInString() const;
        
    private:
        DataFileContentInformation(const DataFileContentInformation&);

        DataFileContentInformation& operator=(const DataFileContentInformation&);
        
    public:

        // ADD_NEW_METHODS_HERE

    private:
        std::vector<std::pair<AString, AString> > m_namesAndValues;
        
        AString m_text;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __DATA_FILE_CONTENT_INFORMATION_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __DATA_FILE_CONTENT_INFORMATION_DECLARE__

} // namespace
#endif  //__DATA_FILE_CONTENT_INFORMATION_H__
