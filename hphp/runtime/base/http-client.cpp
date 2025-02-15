/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include "hphp/runtime/base/http-client.h"
#include "hphp/runtime/server/server-stats.h"
#include "hphp/runtime/base/curl-tls-workarounds.h"
#include "hphp/runtime/base/execution-context.h"
#include "hphp/util/configs/http.h"
#include "hphp/util/timer.h"
#include <curl/easy.h>
#include <vector>
#include "hphp/util/logger.h"

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

//so that curl_global_init() is called ahead of time, avoiding crash
struct StaticInitializer {
  StaticInitializer() {
    curl_global_init(CURL_GLOBAL_ALL);
  }
};
static StaticInitializer s_initCurl;

HttpClient::HttpClient(int timeout /* = 5 */, int maxRedirect /* = 1 */,
                       bool use11 /* = true */, bool decompress /* = false */)
  : m_timeout(timeout), m_maxRedirect(maxRedirect), m_use11(use11),
    m_decompress(decompress), m_response(nullptr), m_responseHeaders(nullptr),
    m_proxyPort(0) {
  if (m_timeout <= 0) {
    m_timeout = RequestInfo::s_requestInfo.getNoCheck()->
      m_reqInjectionData.getSocketDefaultTimeout();
  }
}

size_t HttpClient::curl_write(char *data, size_t size, size_t nmemb,
                              void *ctx) {
  return ((HttpClient*)ctx)->write(data, size, nmemb);
}
size_t HttpClient::write(char *data, size_t size, size_t nmemb) {
  size_t length = size * nmemb;
  if (length > 0 && m_response) {
    m_response->append(data, (int)length);
  }
  return length;
}

size_t HttpClient::curl_header(char *data, size_t size, size_t nmemb,
                               void *ctx) {
  return ((HttpClient*)ctx)->header(data, size, nmemb);
}
size_t HttpClient::header(char *data, size_t size, size_t nmemb) {
  size_t length = size * nmemb;
  if (length > 2 && data[length - 2] == '\r' && data[length - 1] == '\n' &&
      m_responseHeaders) {
    m_responseHeaders->push_back(String(data, length - 2, CopyString));
  }
  return length;
}

void HttpClient::auth(const std::string& username, const std::string& password,
                      bool /*basic*/ /* = true */) {
  m_basic = true;
  m_username = username;
  m_password = password;
}

void HttpClient::proxy(const std::string &host, int port,
                       const std::string &username /* = "" */,
                       const std::string &password /* = "" */) {
  m_proxyHost = host;
  m_proxyPort = port;
  m_proxyUsername = username;
  m_proxyPassword = password;
}

void HttpClient::setHttpsProxy(const std::string& proxyCaBundle,
                      const std::string& proxySSLCertPath /* = "" */,
                      const std::string& proxySSLKeyPath /* = "" */) {
  m_proxyCaBundle = proxyCaBundle;
  m_proxySSLCertPath = proxySSLCertPath;
  m_proxySSLKeyPath = proxySSLKeyPath;
}

int HttpClient::get(const char *url, StringBuffer &response,
                    const HeaderMap *requestHeaders /* = NULL */,
                    req::vector<String> *responseHeaders /* = NULL */) {
  return request(nullptr,
                 url, nullptr, 0, response, requestHeaders, responseHeaders);
}

int HttpClient::post(const char *url, const char *data, size_t size,
                     StringBuffer &response,
                     const HeaderMap *requestHeaders /* = NULL */,
                     req::vector<String> *responseHeaders /* = NULL */) {
  return request(nullptr,
                 url, data, size, response, requestHeaders, responseHeaders);
}

const StaticString
  s_ssl("ssl"),
  s_tls("tls"),
  s_verify_peer("verify_peer"),
  s_verify_peer_name("verify_peer_name"),
  s_capath("capath"),
  s_cafile("cafile"),
  s_local_cert("local_cert"),
  s_passphrase("passphrase"),
  s_http("http"),
  s_header("header");

int HttpClient::request(const char* verb,
                     const char *url, const char *data, size_t size,
                     StringBuffer &response, const HeaderMap *requestHeaders,
                     req::vector<String> *responseHeaders) {
  SlowTimer timer(Cfg::Http::SlowQueryThreshold, "curl", url);

  m_response = &response;

  char error_str[CURL_ERROR_SIZE + 1];
  memset(error_str, 0, sizeof(error_str));

  CURL *cp = curl_easy_init();
  curl_easy_setopt(cp, CURLOPT_URL,               url);
  curl_easy_setopt(cp, CURLOPT_WRITEFUNCTION,     curl_write);
  curl_easy_setopt(cp, CURLOPT_WRITEDATA,         (void*)this);
  curl_easy_setopt(cp, CURLOPT_ERRORBUFFER,       error_str);
  curl_easy_setopt(cp, CURLOPT_NOPROGRESS,        1);
  curl_easy_setopt(cp, CURLOPT_VERBOSE,           0);
  curl_easy_setopt(cp, CURLOPT_NOSIGNAL,          1);
  curl_easy_setopt(cp, CURLOPT_DNS_USE_GLOBAL_CACHE, 0); // for thread-safe
  curl_easy_setopt(cp, CURLOPT_DNS_CACHE_TIMEOUT, 120);
  curl_easy_setopt(cp, CURLOPT_NOSIGNAL, 1); // for multithreading mode
  curl_easy_setopt(cp, CURLOPT_SSL_VERIFYPEER,    1);
  // For libcurl the VERIFYHOST "true"/enabled value is '2', NOT '1'!
  // If libcurl is built with NSS, VERIFYPEER =0 forces VERIFYHOST to =0
  curl_easy_setopt(cp, CURLOPT_SSL_VERIFYHOST,    2);
  curl_easy_setopt(cp, CURLOPT_SSL_CTX_FUNCTION, curl_tls_workarounds_cb);
  curl_easy_setopt(cp, CURLOPT_USE_SSL, m_use_ssl);
  curl_easy_setopt(cp, CURLOPT_SSLVERSION, m_sslversion);

  /*
   * cipher list varies according to SSL library, and "ALL" is for OpenSSL
   */
  curl_version_info_data *cver = curl_version_info(CURLVERSION_NOW);
  if (cver && cver->ssl_version && strstr(cver->ssl_version, "OpenSSL")) {
    curl_easy_setopt(cp, CURLOPT_SSL_CIPHER_LIST, "ALL");
  }

  curl_easy_setopt(cp, CURLOPT_TIMEOUT,           m_timeout);
  if (m_maxRedirect > 1) {
    curl_easy_setopt(cp, CURLOPT_FOLLOWLOCATION,    1);
    curl_easy_setopt(cp, CURLOPT_MAXREDIRS,         m_maxRedirect);
  } else {
    curl_easy_setopt(cp, CURLOPT_FOLLOWLOCATION,    0);
  }
  if (!m_use11) {
    curl_easy_setopt(cp, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  }
  if (m_decompress) {
    curl_easy_setopt(cp, CURLOPT_ENCODING, "");
  }

  if (!m_username.empty()) {
    curl_easy_setopt(cp, CURLOPT_HTTPAUTH,
                     m_basic ? CURLAUTH_BASIC : CURLAUTH_DIGEST);
    curl_easy_setopt(cp, CURLOPT_USERNAME, m_username.c_str());
    curl_easy_setopt(cp, CURLOPT_PASSWORD, m_password.c_str());
  }

  if (!m_proxyHost.empty() && m_proxyPort) {
    curl_easy_setopt(cp, CURLOPT_PROXY,     m_proxyHost.c_str());
    curl_easy_setopt(cp, CURLOPT_PROXYPORT, m_proxyPort);
    if (!m_proxyUsername.empty()) {
      curl_easy_setopt(cp, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
      curl_easy_setopt(cp, CURLOPT_PROXYUSERNAME, m_proxyUsername.c_str());
      curl_easy_setopt(cp, CURLOPT_PROXYPASSWORD, m_proxyPassword.c_str());
    }

    if (!m_proxyCaBundle.empty()) {
      curl_easy_setopt(cp, CURLOPT_PROXYTYPE, CURLPROXY_HTTPS);
      curl_easy_setopt(
          cp, CURLOPT_PROXY_CAINFO, m_proxyCaBundle.c_str());
      if (!m_proxySSLCertPath.empty() && !m_proxySSLKeyPath.empty()) {
        curl_easy_setopt(
            cp, CURLOPT_PROXY_SSLCERT, m_proxySSLCertPath.c_str());
        curl_easy_setopt(
            cp, CURLOPT_PROXY_SSLKEY, m_proxySSLKeyPath.c_str());
      }
    }

  }

  curl_slist *slist = nullptr;
  if (requestHeaders) {
    for (HeaderMap::const_iterator iter = requestHeaders->begin();
         iter != requestHeaders->end(); ++iter) {
      for (unsigned int i = 0; i < iter->second.size(); i++) {
        String header = iter->first + ": " + iter->second[i];
        slist = curl_slist_append(slist, header.data());
      }
    }
  if (m_stream_context_options[s_http].isArray()) {
    const Array http = m_stream_context_options[s_http].toArray();
    if (http.exists(s_header)) {
      slist = curl_slist_append(slist,
                                http[s_header].toString().data());
    }
  }
  if (slist) {
      curl_easy_setopt(cp, CURLOPT_HTTPHEADER, slist);
    }
  }

  if (data && size) {
    curl_easy_setopt(cp, CURLOPT_POST,          1);
    curl_easy_setopt(cp, CURLOPT_POSTFIELDS,    data);
    if (size <= 0x7fffffffLL) {
      curl_easy_setopt(cp, CURLOPT_POSTFIELDSIZE, size);
    } else {
      curl_easy_setopt(cp, CURLOPT_POSTFIELDSIZE_LARGE, size);
    }
  }

  if (verb != nullptr) {
    curl_easy_setopt(cp, CURLOPT_CUSTOMREQUEST, verb);

    if (strcasecmp(verb, "HEAD") == 0) {
      curl_easy_setopt(cp, CURLOPT_NOBODY, 1);
    }
  }

  if (responseHeaders) {
    m_responseHeaders = responseHeaders;
    curl_easy_setopt(cp, CURLOPT_HEADERFUNCTION, curl_header);
    curl_easy_setopt(cp, CURLOPT_WRITEHEADER, (void*)this);
  }

  if (m_stream_context_options[s_ssl].isArray() ||
      m_stream_context_options[s_tls].isArray()) {
    const Array ssl = m_stream_context_options[s_ssl].isArray() ? \
                      m_stream_context_options[s_ssl].toArray() : \
                      m_stream_context_options[s_tls].toArray();
    if (ssl.exists(s_verify_peer)) {
      curl_easy_setopt(cp, CURLOPT_SSL_VERIFYPEER,
                       ssl[s_verify_peer].toBoolean());
    }
    if (ssl.exists(s_verify_peer_name)) {
      // For libcurl VERIFYHOST the enable/"true" value is '2', NOT '1'!
      curl_easy_setopt(cp, CURLOPT_SSL_VERIFYHOST,
                       ssl[s_verify_peer_name].toBoolean() ? \
                       2 : 0);
    }
    if (ssl.exists(s_capath)) {
      curl_easy_setopt(cp, CURLOPT_CAPATH,
                       ssl[s_capath].toString().data());
    }
    if (ssl.exists(s_cafile)) {
      curl_easy_setopt(cp, CURLOPT_CAINFO,
                       ssl[s_cafile].toString().data());
    }
    if (ssl.exists(s_local_cert)) {
      curl_easy_setopt(cp, CURLOPT_SSLKEY,
                       ssl[s_local_cert].toString().data());
      curl_easy_setopt(cp, CURLOPT_SSLKEYTYPE, "PEM");
    }
    if (ssl.exists(s_passphrase)) {
      curl_easy_setopt(cp, CURLOPT_KEYPASSWD,
                       ssl[s_passphrase].toString().data());
    }
  }

  long code = 0;
  {
    IOStatusHelper io("http", url);
    CURLcode error_no = curl_easy_perform(cp);
    if (error_no != CURLE_OK) {
      m_error = error_str;
    } else {
      curl_easy_getinfo(cp, CURLINFO_RESPONSE_CODE, &code);
    }
  }

  set_curl_statuses(cp, url);

  if (slist) {
    curl_slist_free_all(slist);
  }

  curl_easy_cleanup(cp);
  return code;
}

///////////////////////////////////////////////////////////////////////////////
}
