// DODSContainerPersistenceFile.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef I_DODSContainerPersistenceFile_h_
#define I_DODSContainerPersistenceFile_h_ 1

#include <string>
#include <map>

using std::string ;
using std::map ;

#include "DODSContainerPersistence.h"

/** @brief implementation of DODSContainerPersistence that represents a
 * way to read container information from a file.
 *
 * This impelementation of DODSContainerPersistence load container information
 * from a file. The name of the file is determined from the dods
 * initiailization file. The key is:
 *
 * DODS.Container.Persistence.File.&lt;name&gt;
 *
 * where &lt;name&gt; is the name of this persistent store.
 *
 * The format of the file is:
 *
 * &lt;symbolic_name&gt; &lt;real_name&gt; &lt;data type&gt;
 *
 * where the &lt;symbolic_name&gt; is the symbolic name of the container, the
 * &lt;real_name&gt; represents the physical location of the data, such as a
 * file, and the &lt;data type&gt; is the type of data being represented,
 * such as netcdf, cedar, etc...
 *
 * One container per line, can not span multiple lines
 *
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSKeys
 */
class DODSContainerPersistenceFile : public DODSContainerPersistence
{
private:
    typedef struct _container
    {
	string _symbolic_name ;
	string _real_name ;
	string _container_type ;
    } container ;
    map< string, DODSContainerPersistenceFile::container * > _container_list ;
    typedef map< string, DODSContainerPersistenceFile::container * >::const_iterator Container_citer ;
    typedef map< string, DODSContainerPersistenceFile::container * >::iterator Container_iter ;

public:
    				DODSContainerPersistenceFile( const string &n );
    virtual			~DODSContainerPersistenceFile() ;

    virtual void		look_for( DODSContainer &d ) ;
    virtual void		add_container( string s_name, string r_name,
					       string type ) ;
    virtual bool		rem_container( const string &s_name ) ;

    virtual void		show_containers( DODSInfo &info ) ;
};

#endif // I_DODSContainerPersistenceFile_h_

// $Log: DODSContainerPersistenceFile.h,v $
// Revision 1.8  2005/03/17 20:37:14  pwest
// implemented rem_container to remove the container from memory, but not from the file. Added documentation for rem_container and show_containers
//
// Revision 1.7  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.6  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.5  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
