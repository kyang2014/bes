// ServerApp.cc

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

#include <signal.h>
#include <unistd.h>

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;
using std::flush ;

#include "ServerApp.h"
#include "ServerExitConditions.h"
#include "TheDODSKeys.h"
#include "SocketListener.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "OPeNDAPServerHandler.h"
#include "DODSException.h"
#include "PPTException.h"
#include "PPTServer.h"
#include "PPTException.h"
#include "SocketException.h"
#include "DODSMemoryManager.h"

ServerApp::ServerApp()
    : _portVal( 0 ),
      _gotPort( false ),
      _unixSocket( "" )
{
}

ServerApp::~ServerApp()
{
}

void
ServerApp::signalTerminate( int sig )
{
    if( sig == SIGINT )
    {
	cout << OPeNDAPApp::TheApplication()->appName() << ":" << getpid()
	     << ": got termination signal, exiting!" << endl ;
	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}

void
ServerApp::signalRestart( int sig )
{
    if( sig == SIGUSR1 )
    {
	cout << OPeNDAPApp::TheApplication()->appName() << ":" << getpid()
	     << ": got restart signal." << endl ;
	exit( SERVER_EXIT_RESTART ) ;
    }
}

void
ServerApp::showUsage()
{
    cout << OPeNDAPApp::TheApplication()->appName()
         << ": -d -v -p [PORT]" << endl ;
    cout << "-d set the server to debugging mode" << endl ;
    cout << "-v echos version and exit" << endl ;
    cout << "-p set port to PORT" << endl ;
    exit( 0 ) ;
}

void
ServerApp::showVersion()
{
  cout << OPeNDAPApp::TheApplication()->appName() << " version 2.0" << endl ;
  exit( 0 ) ;
}

int
ServerApp::initialize( int argc, char **argv )
{
    int retVal = OPeNDAPBaseApp::initialize( argc, argv ) ;
    if( retVal != 0 )
	return retVal ;

    cout << "Trying to register SIGINT ... " << flush ;
    if( signal( SIGINT, signalTerminate ) == SIG_ERR )
    {
	cerr << "FAILED: Can not register SIGINT signal handler" << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    else
	cout << "OK" << endl ;

    cout << "Trying to register SIGUSR1 ... " << flush ;
    if( signal( SIGUSR1, signalRestart ) == SIG_ERR )
    {
	cerr << "FAILED: Can not register SIGUSR1 signal handler" << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    else
	cout << "OK" << endl ;

    int c = 0 ;

    while( ( c = getopt( argc, argv, "dvp:" ) ) != EOF )
    {
	switch( c )
	{
	    case 'p':
		_portVal = atoi( optarg ) ;
		_gotPort = true ;
		break ;
	    case 'd':
		setDebug( true ) ;
		break ;
	    case 'v':
		showVersion() ;
		break ;
	    case '?':
		showUsage() ;
		break ;
	}
    }

    if( !_gotPort )
    {
	showUsage() ;
    }

    bool found = false ;
    string key = "DODS.ServerUnixSocket" ;
    _unixSocket = TheDODSKeys::TheKeys()->get_key( key, found ) ;
    if( !found || _unixSocket == "" )
    {
	cout << "Unable to determine unix socket" << endl ;
	cout << "Please " << key << " in the opendap initialization file"
	     << endl ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }

    return 0 ;
}

int
ServerApp::run()
{
    try
    {
	DODSMemoryManager::initialize_memory_pool() ;

	SocketListener listener ;

	TcpSocket ts( _portVal ) ;
	listener.listen( &ts ) ;

	UnixSocket us( _unixSocket ) ;
	listener.listen( &us ) ;

	OPeNDAPServerHandler handler ;

	PPTServer ps( &handler, &listener ) ;
	ps.initConnection() ;
	ps.closeConnection() ;
    }
    catch( PPTException &pe )
    {
	cerr << "caught PPTException" << endl ;
	cerr << pe.getMessage() << endl ;
	return 1 ;
    }
    catch( SocketException &se )
    {
	cerr << "caught SocketException" << endl ;
	cerr << se.getMessage() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "caught unknown exception" << endl ;
	return 1 ;
    }

    return 0 ;
}

int
main( int argc, char **argv )
{
    try
    {
	ServerApp app ;
	return app.main( argc, argv ) ;
    }
    catch( DODSException &e )
    {
	cerr << "Caught unhandled exception: " << endl ;
	cerr << e.get_error_description() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Caught unhandled, unknown exception" << endl ;
	return 1 ;
    }
    return 0 ;
}

