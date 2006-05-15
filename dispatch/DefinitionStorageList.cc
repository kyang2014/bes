// DefinitionStorageList.cc

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

#include <iostream>

using std::endl ;

#include "DefinitionStorageList.h"
#include "DefinitionStorage.h"
#include "DefinitionStorageException.h"
#include "DODSDefine.h"
#include "DODSInfo.h"

DefinitionStorageList *DefinitionStorageList::_instance = 0 ;

DefinitionStorageList::DefinitionStorageList()
    : _first( 0 )
{
}

DefinitionStorageList::~DefinitionStorageList()
{
    DefinitionStorageList::persistence_list *pl = _first ;
    while( pl )
    {
	if( pl->_persistence_obj )
	{
	    delete pl->_persistence_obj ;
	}
	DefinitionStorageList::persistence_list *next = pl->_next ;
	delete pl ;
	pl = next ;
    }
}

/** @brief Add a persistent store to the list
 *
 * Each persistent store has a name. If a persistent store already exists in
 * the list with that name then the persistent store is not added. Otherwise
 * the store is added to the list.
 *
 * The persistent stores are searched in the order in which they were added.
 *
 * @param cp persistent store to add to the list
 * @return true if successfully added, false otherwise
 * @see DefinitionStorage
 */
bool
DefinitionStorageList::add_persistence( DefinitionStorage *cp )
{
    bool ret = false ;
    if( !_first )
    {
	_first = new DefinitionStorageList::persistence_list ;
	_first->_persistence_obj = cp ;
	_first->_next = 0 ;
	ret = true ;
    }
    else
    {
	DefinitionStorageList::persistence_list *pl = _first ;
	bool done = false ;
	while( done == false )
	{
	    if( pl->_persistence_obj->get_name() != cp->get_name() )
	    {
		if( pl->_next )
		{
		    pl = pl->_next ;
		}
		else
		{
		    pl->_next = new DefinitionStorageList::persistence_list ;
		    pl->_next->_persistence_obj = cp ;
		    pl->_next->_next = 0 ;
		    done = true ;
		    ret = true ;
		}
	    }
	    else
	    {
		done = true ;
		ret = false ;
	    }
	}
    }
    return ret ;
}

/** @brief remove a persistent store from the list
 *
 * Removes the named persistent store from the list.
 *
 * @param persist_name name of the persistent store to be removed
 * @return true if successfully removed, false otherwise
 * @see DefinitionStorage
 */
bool
DefinitionStorageList::del_persistence( const string &persist_name )
{
    bool ret = false ;
    DefinitionStorageList::persistence_list *pl = _first ;
    DefinitionStorageList::persistence_list *last = 0 ;

    bool done = false ;
    while( done == false )
    {
	if( pl )
	{
	    if( pl->_persistence_obj->get_name() == persist_name )
	    {
		ret = true ;
		done = true ;
		if( pl == _first )
		{
		    _first = _first->_next ;
		}
		else
		{
		    last->_next = pl->_next ;
		}
		delete pl->_persistence_obj ;
		delete pl ;
	    }
	    else
	    {
		last = pl ;
		pl = pl->_next ;
	    }
	}
	else
	{
	    done = true ;
	}
    }

    return ret ;
}

/** @brief find the persistence store with the given name
 *
 * Returns the persistence store with the given name
 *
 * @param persist_name name of the persistent store to be found
 * @return the persistence store DefinitionStorage
 * @see DefinitionStorage
 */
DefinitionStorage *
DefinitionStorageList::find_persistence( const string &persist_name )
{
    DefinitionStorage *ret = NULL ;
    DefinitionStorageList::persistence_list *pl = _first ;
    bool done = false ;
    while( done == false )
    {
	if( pl )
	{
	    if( persist_name == pl->_persistence_obj->get_name() )
	    {
		ret = pl->_persistence_obj ;
		done = true ;
	    }
	    else
	    {
		pl = pl->_next ;
	    }
	}
	else
	{
	    done = true ;
	}
    }
    return ret ;
}

/** @brief look for the specified definition in the list of defintion stores.
 *
 * Looks for a definition with the given name in the order in which
 * definition stores were added to the definition storage list.
 *
 * @param def_name name of the definition to find
 * @return defintion with the given name, null otherwise
 * @see DefinitionStorage
 * @see DODSDefine
 */
DODSDefine *
DefinitionStorageList::look_for( const string &def_name )
{
    DODSDefine *ret_def = NULL ;
    DefinitionStorageList::persistence_list *pl = _first ;
    bool done = false ;
    while( done == false )
    {
	if( pl )
	{
	    ret_def = pl->_persistence_obj->look_for( def_name ) ;
	    if( ret_def )
	    {
		done = true ;
	    }
	    else
	    {
		pl = pl->_next ;
	    }
	}
	else
	{
	    done = true ;
	}
    }
    return ret_def ;
}

/** @brief show information for each definition in each persistence store
 *
 * For each definition in each persistent store, add infomation about each of
 * those definitions. The information added to the information object
 * includes the persistent store name, in the order the persistent
 * stores are searched, followed by a line for each definition within that
 * persistent store which includes the name of the definition, information
 * about each container used by that definition, the aggregation server
 * being used and the aggregation command being used if aggregation is
 * specified.
 *
 * @param info object to store the definition and persistent store information
 * @see DODSInfo
 */
void
DefinitionStorageList::show_definitions( DODSInfo &info )
{
    DefinitionStorageList::persistence_list *pl = _first ;
    if( !pl )
    {
	info.add_data( "No persistence stores available\n" ) ;
    }
    bool first = true ;
    while( pl )
    {
	if( !first )
	{
	    // separate each store with a blank line
	    info.add_data( "\n" ) ;
	}
	first = false ;
	pl->_persistence_obj->show_definitions( info ) ;
	pl = pl->_next ;
    }
}

DefinitionStorageList *
DefinitionStorageList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new DefinitionStorageList ;
    }
    return _instance ;
}

