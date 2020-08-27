// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

// Copyright (c) 2020 OPeNDAP, Inc.
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

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef I_HTTP_NAME_H
#define I_HTTP_NAME_H 1

#define HTTP_PROTOCOL "http://"
#define HTTPS_PROTOCOL "https://"

#define HTTP_MIMELIST_KEY "Http.MimeTypes"
#define HTTP_PROXYPROTOCOL_KEY "Http.ProxyProtocol"
#define HTTP_PROXYHOST_KEY "Http.ProxyHost"
#define HTTP_PROXYPORT_KEY "Http.ProxyPort"
#define HTTP_PROXYAUTHTYPE_KEY "Http.ProxyAuthType"
#define HTTP_PROXYUSER_KEY "Http.ProxyUser"
#define HTTP_PROXYPASSWORD_KEY "Http.ProxyPassword"
#define HTTP_PROXYUSERPW_KEY "Http.ProxyUserPW"
#define HTTP_USE_INTERNAL_CACHE_KEY "Http.UseInternalCache"

#define HTTP_CACHE_DIR_KEY "Http.Cache.dir"
#define HTTP_CACHE_PREFIX_KEY "Http.Cache.prefix"
#define HTTP_CACHE_SIZE_KEY "Http.Cache.size"

#define HTTP_COOKIES_FILE_KEY "Http.Cookies.File"
#define HTTP_DEFAULT_COOKIES_FILE "/tmp/.hyrax-cookies"

#define HTTP_TARGET_URL_KEY "target_url"
#define HTTP_URL_BASE_KEY "url_base"
#define HTTP_QUERY_STRING_KEY "query_string"
#define HTTP_URL_BASE_KEY "url_base"
#define HTTP_INGEST_TIME_KEY "ingest_time"

#define HTTP_NO_RETRY_URL_REGEX_KEY "Http.No.Retry.Regex"
#define HTTP_CACHE_EFFECTIVE_URLS_KEY "Http.cache.effective.urls"
#define HTTP_CACHE_EFFECTIVE_URLS_SKIP_REGEX_KEY "Http.cache.effective.urls.skip.regex.pattern"


#define AMS_EXPIRES_HEADER_KEY "X-Amz-Expires"
#define AWS_DATE_HEADER_KEY "X-Amz-Date"
#define CLOUDFRONT_EXPIRES_HEADER_KEY "Expires"

#define HTTP_MODULE "http"

#endif // I_HTTP_NAME_H