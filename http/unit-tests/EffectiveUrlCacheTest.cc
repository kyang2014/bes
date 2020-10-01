// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <time.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <BESCatalogList.h>
#include <TheBESKeys.h>
#include "EffectiveUrlCache.h"
#include "HttpNames.h"

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace http {

    class EffectiveUrlCacheTest: public CppUnit::TestFixture {
    private:

        /**
         *
         */
        void show_file(string filename)
        {
            ifstream t(filename.c_str());

            if (t.is_open()) {
                string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
                t.close();
                cerr << endl << "#############################################################################" << endl;
                cerr << "file: " << filename << endl;
                cerr <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
                cerr << file_content << endl;
                cerr << "#############################################################################" << endl;
            }
            else {
                cerr << "FAILED TO OPEN FILE: " << filename << endl;
            }
        }

        std::string get_file_as_string(string filename)
        {
            ifstream t(filename.c_str());

            if (t.is_open()) {
                string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
                t.close();
                if(Debug) cerr << endl << "#############################################################################" << endl;
                if(Debug) cerr << "file: " << filename << endl;
                if(Debug) cerr <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
                if(Debug) cerr << file_content << endl;
                if(Debug) cerr << "#############################################################################" << endl;
                return file_content;
            }
            else {
                cerr << "FAILED TO OPEN FILE: " << filename << endl;
                CPPUNIT_ASSERT(false);
                return "";
            }
        }

        /**
         *
         */
        string get_data_file_url(string name){
            string prolog = string(__func__) + "() - ";
            string data_file = BESUtil::assemblePath(d_data_dir,name);
            if(debug) cerr << prolog << "data_file: " << data_file << endl;
            if(Debug) show_file(data_file);

            string data_file_url = "file://" + data_file;
            if(debug) cerr << prolog << "data_file_url: " << data_file_url << endl;
            return data_file_url;
        }

        void purge_test_cache(){
            if(Debug) cerr << "Purging cache!" << endl;
            string cache_dir;
            bool found;
            TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found);
            if(found){
                if(Debug) cerr << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
                if(Debug) cerr << "Purging " << cache_dir << endl;
                string cmd = "exec rm -r "+ BESUtil::assemblePath(cache_dir,"/*");
                system(cmd.c_str());
            }
        }

    public:
        string d_data_dir;

        // Called once before everything gets tested
        EffectiveUrlCacheTest()
        {
            d_data_dir = TEST_DATA_DIR;;
            cerr << "data_dir: " << d_data_dir << endl;
        }

        // Called at the end of the test
        ~EffectiveUrlCacheTest()
        {
        }

        // Called before each test
        void setUp()
        {
            if(Debug) cerr << endl << "setUp() - BEGIN" << endl;
            string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
            if(Debug) cerr << "setUp() - Using BES configuration: " << bes_conf << endl;
            if (bes_debug) show_file(bes_conf);
            TheBESKeys::ConfigFile = bes_conf;

            if (bes_debug) BESDebug::SetUp("cerr,wl,bes,http");

            if(purge_cache) purge_test_cache();

            if(Debug) cerr << "setUp() - END" << endl;
        }

        // Called after each test
        void tearDown()
        {
        }


/*##################################################################################################*/
/* TESTS BEGIN */


        void cache_test_00() {
            if(debug) cerr << __func__ << "() - BEGIN" << endl;
            string source_url;
            string value;
            http::url *effective_url;
            try {
                string src_url_00 = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20040107v7.nc";
                http::url *effective_url_00 = new http::url("https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899");
                EffectiveUrlCache::TheCache()->add(src_url_00,effective_url_00);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 1);

                string src_url_01 = "http://test.opendap.org/data/httpd_catalog/READTHIS";
                http::url *effective_url_01 = new http::url("https://test.opendap.org/data/httpd_catalog/READTHIS");
                EffectiveUrlCache::TheCache()->add(src_url_01,effective_url_01);
                CPPUNIT_ASSERT( EffectiveUrlCache::TheCache()->d_effective_urls.size() == 2);

                effective_url = EffectiveUrlCache::TheCache()->get(src_url_00);
                CPPUNIT_ASSERT(effective_url == effective_url_00);


            }
            catch (BESError be){
                stringstream msg;
                msg << __func__ << "() - ERROR! Caught BESError. Message: " << be.get_message() << endl;
                CPPUNIT_FAIL(msg.str());

            }

        }

        string get_amz_date(const time_t &da_time){
            string amz_date_format("%Y%m%dT%H%M%SZ"); // "20200808T032623Z";
            struct tm *dttm;
            dttm = gmtime (&da_time);
            int value;

            stringstream amz_date;
            value = dttm->tm_year + 1900 ;
            amz_date << value;

            value = dttm->tm_mon + 1 ;
            amz_date << (value<10?"0":"") << value;

            value = dttm->tm_mday;
            amz_date << (value<10?"0":"") << value;

            amz_date << "T";

            value = dttm->tm_hour;
            amz_date << (value<10?"0":"") << value;

            value = dttm->tm_min;
            amz_date << (value<10?"0":"") << value;

            value = dttm->tm_sec;
            amz_date << (value<10?"0":"") << value;

            amz_date << "Z";

            if(debug) cout << "amz_date: " << amz_date.str() << " len: " << amz_date.str().length() << endl;
            return amz_date.str();
        }


/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( EffectiveUrlCacheTest );

        CPPUNIT_TEST(cache_test_00);

    CPPUNIT_TEST_SUITE_END();
};

    CPPUNIT_TEST_SUITE_REGISTRATION(EffectiveUrlCacheTest);

} // namespace httpd_catalog

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dbDP");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                cerr << "debug enabled" << endl;
                break;
            case 'D':
                Debug = true;  // Debug is a static global
                cerr << "Debug enabled" << endl;
                break;
            case 'b':
                bes_debug = true;  // debug is a static global
                cerr << "bes_debug enabled" << endl;
                break;
            case 'P':
                purge_cache = true;  // purge_cache is a static global
                cerr << "purge_cache enabled" << endl;
                break;
            default:
                break;
        }

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
            test = http::EffectiveUrlCacheTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
