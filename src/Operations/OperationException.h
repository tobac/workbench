#ifndef __OPERATION_EXCEPTION_H__
#define __OPERATION_EXCEPTION_H__

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


#include <exception>

#include "CaretException.h"

namespace caret {

/// An exception thrown during command processing.
class OperationException : public CaretException {

public:
    OperationException();

    OperationException(const CaretException& e);

    OperationException(const AString& s);

    OperationException(const OperationException& e);
    
    OperationException& operator=(const OperationException& e);
    
    virtual ~OperationException() throw();
    
private:
        
    void initializeMembersOperationException();
};

} // namespace

#endif // __OPERATION_EXCEPTION_H__
