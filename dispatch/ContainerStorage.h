// ContainerStorage.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef ContainerStorage_h_
#define ContainerStorage_h_ 1

#include <string>

using std::string ;

class DODSContainer ;
class DODSInfo ;

/** @brief provides persistent storage for data storage information
 * represented by a symbolic name.
 *
 * An implementation of the abstract interface ContainerStorage
 * provides storage for information about accessing data of different types.
 * The information is represented by a symbolic name. A user can request a
 * symbolic name that represents a certain data file of a certain data type.
 * For example, a symbolic name 'nc1' could represent the netcdf file
 * /usr/apache/htdocs/netcdf/datfile01.cdf.
 *
 * An instance of a derived implementation has a name associated with it, in
 * case that there are multiple ways in which the information can be stored.
 * For example, the main persistent storage for containers could be a mysql
 * database, but a user could store temporary information in different files.
 * If the user wishes to remove one of these persistence stores they would
 * request that a named ContainerStorage object be removed from the
 * list.
 * 
 * @see DODSContainer
 * @see ContainerStorageList
 */
class ContainerStorage
{
protected:
    string		_my_name ;

public:
    /** @brief create an instance of ContainerStorage with the give
     * name.
     *
     * @param name name of this persistence store
     */
    				ContainerStorage( const string &name )
				    : _my_name( name ) {} ;

    virtual 			~ContainerStorage() {} ;

    /** @brief retrieve the name of this persistent store
     *
     * @return name of this persistent store.
     */
    virtual string		get_name() { return _my_name ; }

    /** @brief looks for a container in this persistent store
     *
     * This method looks for a container with the name specified in the given
     * container and fills in the information in the given container.
     *
     * @param d The container to look for and, if found, the information is
     * filled in.
     */
    virtual void 		look_for( DODSContainer &d ) = 0 ;

    /** @brief adds a container with the provided information
     *
     * This method adds a container to the persistence store with the
     * specified information.
     *
     * @param s_name symbolic name for the container
     * @param r_name real name for the container
     * @param type type of data represented by this container
     */
    virtual void		add_container( const string &s_name,
                                               const string &r_name,
					       const string &type ) = 0 ;

    /** @brief removes a container with the given symbolic name
     *
     * This method removes a container to the persistence store with the
     * given symbolic name. It deletes the container.
     *
     * @param s_name symbolic name for the container
     * @return true if successfully removed and false otherwise
     */
    virtual bool		del_container( const string &s_name ) = 0 ;

    /** @brief show the containers stored in this persistent store
     *
     * Add information to the passed information object about each of the
     * containers stored within this persistent store. The information
     * added to the passed information objects includes the name of this
     * persistent store on the first line followed by the symbolic name,
     * real name and data type for each container, one per line.
     *
     * @param info information object to store the information in
     */
    virtual void		show_containers( DODSInfo &info ) = 0 ;
};

#endif // ContainerStorage_h_

