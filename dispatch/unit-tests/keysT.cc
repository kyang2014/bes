// keysT.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <cstdlib>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

#include "TheBESKeys.h"
#include "BESError.h"

#include <GetOpt.h>
#include <debug.h>

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool debug_2 = false;
string HR = "*****************************************";
string BEGIN ="() - BEGIN";
string END ="() - END";


#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);

using namespace CppUnit;

//string KeyFile;

class keysT: public TestFixture {
private:

public:
    keysT()
    {
    }
    ~keysT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }


    /**
     *
     */
    void bad_keys_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/bad_keys1.ini";
        TheBESKeys::ConfigFile = bes_conf;
        try {
            TheBESKeys::TheKeys();
            CPPUNIT_FAIL("loaded keys, should have failed");
        }
        catch (BESError &e) {
            if(debug) cout << "Unable to create BESKeys, which is good. Message: " <<  e.get_message() << endl;
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void good_keys_test() {
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/keys_test.ini";
        TheBESKeys::ConfigFile = bes_conf;

        if(debug) cout << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl;

        try {
            TheBESKeys::TheKeys();
        }
        catch (BESError &e) {
            //cerr << "Error: " << e.get_message() << endl;
            cerr << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl;
            CPPUNIT_FAIL("Unable to create BESKeys: " + e.get_message());
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_keys_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        bool found = false;
        string ret = "";
        for (int i = 1; i < 5; i++) {
            char key[32];
            sprintf(key, "BES.KEY%d", i);
            char val[32];
            sprintf(val, "val%d", i);
            if(debug) cout << "Looking for " << key << " with value '" << val << "'" << endl;
            ret = "";
            TheBESKeys::TheKeys()->get_value(key, ret, found);
            CPPUNIT_ASSERT(found);
            CPPUNIT_ASSERT(!ret.empty());
            CPPUNIT_ASSERT(ret == val);
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_multi_for_single_value_key_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        bool found = false;
        vector<string> vals;
        TheBESKeys::TheKeys()->get_values("BES.KEY1", vals, found);
        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT(vals.size() == 1);
        CPPUNIT_ASSERT(vals[0] == "val1");
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_multi_valued_key_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        bool found = false;
        vector<string> vals;
        TheBESKeys::TheKeys()->get_values("BES.KEY.MULTI", vals, found);
        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT(vals.size() == 5);
        for (int i = 0; i < 5; i++) {
            char val[32];
            sprintf(val, "val_multi_%d", i);
            if(debug) cout << "Looking for value '" << val << "'"  << endl;
            CPPUNIT_ASSERT(vals[i] == val);
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_value_on_multi_valued_key_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        try {
            TheBESKeys::TheKeys()->get_value("BES.KEY.MULTI", ret, found);
        }
        catch (BESError &e) {
            if(debug) cout << "Using TheBESKeys::get_value() on a multi-value key failed, good. Message: " << e.get_message() << endl;
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_missing_key_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        TheBESKeys::TheKeys()->get_value("BES.NOTFOUND", ret, found);
        CPPUNIT_ASSERT(found == false);
        CPPUNIT_ASSERT(ret.empty());
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_empty_valued_key_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        TheBESKeys::TheKeys()->get_value("BES.KEY5", ret, found);
        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT(ret.empty());
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void get_empty_valued_key_from_included_file_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        TheBESKeys::TheKeys()->get_value("BES.KEY6", ret, found);
        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT(ret.empty());
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void set_bad_key_missing_equals_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        try {
            TheBESKeys::TheKeys()->set_key("BES.NOEQS");
            CPPUNIT_ASSERT(!"TheBESKeys::set_key() successful, should have failed");
        }
        catch (BESError &e) {
            if(debug) cout << "Unable to set the key, which is good. Message: " << e.get_message() << endl;
            CPPUNIT_ASSERT(true);
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void set_bad_key_double_equals_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        try {
            TheBESKeys::TheKeys()->set_key("BES.2EQS=val1=val2");
            found = false;
            TheBESKeys::TheKeys()->get_value("BES.2EQS", ret, found);
            CPPUNIT_ASSERT(ret == "val1=val2");
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"failed to set key where value has multiple =");
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void set_key7_to_val7_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        try {
            TheBESKeys::TheKeys()->set_key("BES.KEY7=val7");
            found = false;
            TheBESKeys::TheKeys()->get_value("BES.KEY7", ret, found);
            CPPUNIT_ASSERT(ret == "val7");
        }
        catch (BESError &e) {
            cerr << e.get_message();
            CPPUNIT_ASSERT(!"unable to set the key");
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void set_key8_to_val8_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string ret = "";
        bool found = false;
        try {
            TheBESKeys::TheKeys()->set_key("BES.KEY8", "val8");
            found = false;
            TheBESKeys::TheKeys()->get_value("BES.KEY8", ret, found);
            CPPUNIT_ASSERT(ret == "val8");
        }
        catch (BESError &e) {
            cerr << e.get_message();
            CPPUNIT_ASSERT(!"unable to set the key");
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void add_val_multi_5_to_bes_key_multi_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        vector<string> vals;
        string ret = "";
        bool found = false;
        try {
            TheBESKeys::TheKeys()->set_key("BES.KEY.MULTI", "val_multi_5", true);
            found = false;
            vals.clear();
            TheBESKeys::TheKeys()->get_values("BES.KEY.MULTI", vals, found);
            CPPUNIT_ASSERT(found);
            CPPUNIT_ASSERT(vals.size() == 6);
            for (int i = 0; i < 6; i++) {
                char val[32];
                sprintf(val, "val_multi_%d", i);
                if(debug) cout << "Looking for value '" << val << "'" << endl;
                CPPUNIT_ASSERT(vals[i] == val);
            }
        }
        catch (BESError &e) {
            cerr << e.get_message();
            CPPUNIT_ASSERT(!"unable to set the key");
        }
        if(debug) cout << __func__ << END << endl;
    }

    void get_keys_from_regex_include_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        vector<string> vals;
        string ret = "";
        bool found = false;
        TheBESKeys::TheKeys()->get_values("BES.KEY.MI", vals, found);
        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT(vals.size() == 3);
        CPPUNIT_ASSERT(vals[0] == "val_multi_2");
        CPPUNIT_ASSERT(vals[1] == "val_multi_1");
        CPPUNIT_ASSERT(vals[2] == "val_multi_3");
        if(debug) cout << __func__ << END << endl;
    }

    void check_keys_test(){
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        vector<string> vals;
        string ret = "";
        bool found = false;
        for (int i = 1; i < 7; i++) {
            char key[32];
            sprintf(key, "BES.KEY%d", i);
            char val[32];
            if (i == 5 || i == 6)
                sprintf(val, "");
            else
                sprintf(val, "val%d", i);
            if(debug) cout << "Looking for " << key << " with value '" << val << "'" << endl;
            ret = "";
            TheBESKeys::TheKeys()->get_value(key, ret, found);
            CPPUNIT_ASSERT(found);
            CPPUNIT_ASSERT(ret == val);
        }
        if(debug) cout << __func__ << "() - END" << endl;
    }


    /**
     *
     */
    void vector_values_test() {
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/keys_test_maps.ini";
        if(debug) cout << "Using TheBESKeys::ConfigFile: " << bes_conf << endl;
        try {
            if(debug) cout << "Calling TheBESKeys()" << endl;
            TheBESKeys besKeys(bes_conf);
            CPPUNIT_ASSERT(besKeys.d_the_keys->size() == 2);

            if(debug) cout << "Keys size: " << besKeys.d_the_keys->size() << endl;
            if(debug) besKeys.dump(cout);

            vector<string> values;
            bool found;
            string vector_key = "BES.TestVector";
            besKeys.get_values(vector_key,values, found);
            CPPUNIT_ASSERT(found);
            CPPUNIT_ASSERT( values.size() == 5 );

            string expected_values[] = {"foo", "bar", "baz", "moo", "noo"};
            for(size_t i=0; i< values.size(); i++){
                CPPUNIT_ASSERT( values[i] == expected_values[i]);
            }
        }
        catch (BESError &e) {
            //cerr << "Error: " << e.get_message() << endl;
            cerr << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl;
            CPPUNIT_FAIL("Unable to create BESKeys: " + e.get_message());
        }
        if(debug) cout << __func__ << END << endl;
    }

    /**
     *
     */
    void map_values_test() {
        if(debug) cout << endl << HR << endl << __func__ << BEGIN << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/keys_test_maps.ini";
        if(debug) cout << "Using TheBESKeys::ConfigFile: " << bes_conf << endl;
        try {
            if(debug) cout << "Calling TheBESKeys()" << endl;
            TheBESKeys besKeys(bes_conf);
            CPPUNIT_ASSERT(besKeys.d_the_keys->size() == 2);

            if(debug) cout << "Keys size: " << besKeys.d_the_keys->size() << endl;
            if(debug) besKeys.dump(cout);

            string map_key = "BES.TestMap";
            map<string, string> values;
            bool found;
            besKeys.get_values(map_key,values, true, found);
            CPPUNIT_ASSERT(found);
            if(debug) cout << "Map Size: " << values.size() << endl;
            CPPUNIT_ASSERT( values.size() == 4 );
            // foo:bar boo:hoo mo:betta weasel:bites
            string expected_map_keys[] {"foo", "boo", "mo",    "weasel"};
            string expected_map_vals[] {"bar", "hoo", "betta", "bites"};

            for(size_t i=0; i<4; i++){
                std::map<string, string>::iterator it;
                it = values.find(expected_map_keys[i]);
                CPPUNIT_ASSERT( it != values.end() );
                CPPUNIT_ASSERT( it->second == expected_map_vals[i] );
            }
        }
        catch (BESError &e) {
            //cerr << "Error: " << e.get_message() << endl;
            cerr << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl;
            CPPUNIT_FAIL("Unable to create BESKeys: " + e.get_message());
        }

        if(debug) cout << __func__ << END << endl;
    }

CPPUNIT_TEST_SUITE( keysT );

    // I broke up the monolithic do_test() function, but
    // I did not resolve the test independence so they must be run in
    // order.
    CPPUNIT_TEST(bad_keys_test);
    CPPUNIT_TEST(good_keys_test);
    CPPUNIT_TEST(get_keys_test);
    CPPUNIT_TEST(get_multi_for_single_value_key_test);
    CPPUNIT_TEST(get_multi_valued_key_test);
    CPPUNIT_TEST(get_value_on_multi_valued_key_test);
    CPPUNIT_TEST(get_missing_key_test);
    CPPUNIT_TEST(get_empty_valued_key_test);
    CPPUNIT_TEST(get_empty_valued_key_from_included_file_test);
    CPPUNIT_TEST(set_bad_key_missing_equals_test);
    CPPUNIT_TEST(set_bad_key_double_equals_test);
    CPPUNIT_TEST(set_key7_to_val7_test);
    CPPUNIT_TEST(set_key8_to_val8_test);
    CPPUNIT_TEST(add_val_multi_5_to_bes_key_multi_test);
    CPPUNIT_TEST(get_keys_from_regex_include_test);
    CPPUNIT_TEST(check_keys_test);
    CPPUNIT_TEST(vector_values_test);
    CPPUNIT_TEST(map_values_test);

    CPPUNIT_TEST_SUITE_END();





    void do_test() {
#if 0
        cout << "*****************************************" << endl;
        cout << "Entered keysT::run" << endl;
        // unused. jhrg 5/24/16 int retVal = 0;

        if (KeyFile != "") {
            TheBESKeys::ConfigFile = KeyFile;
            try {
                TheBESKeys::TheKeys()->dump(cout);
            }
            catch (BESError &e) {
                cout << "unable to create BESKeys:" << endl;
                cout << e.get_message() << endl;
            }
            catch (...) {
                cout << "unable to create BESKeys: unkown exception caught" << endl;
            }
        }
#endif

    }








};

CPPUNIT_TEST_SUITE_REGISTRATION(keysT);

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dDh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: keysT has the following tests:" << endl;
            const std::vector<Test*> &tests = keysT::suite()->getTests();
            unsigned int prefix_len = keysT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
    }


    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = keysT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            i++;
        }
    }

    return wasSuccessful ? 0 : 1;
}
