/*
	Copyright (C) 2017  YLoader.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "token.h"

void Token::update(const std::wstring& symbol) {
  yloader::Lock lock(mx);

  if (!lastSuccessfulUpdate.isNotADateTime()) {
    yloader::TimeDuration d = yloader::LocalTimeSec() - lastSuccessfulUpdate;
    if (d.seconds() < 60) {
      LOG(log_debug,
          _T( "Token update request made within 60 seconds of last one, ignoring"))
      return;
    }
  }

  int wait = 0;
  LOG(log_debug, _T("Updating Yahoo token"))
  static const boost::regex CRUMB_REGEX(
      "CrumbStore\":{\"crumb\":\"([^\"]+)\"}");
  static std::wstring URL(_T("/quote/%1%?p=%1%"));
  static const std::string COOKIE_HEADER("Set-Cookie");
  static const std::wstring SERVER = _T("finance.yahoo.com");

  std::wstring url = yloader::format(URL, urlencode(symbol));

  while (crumb.empty() || crumb.find_first_of("\\u") != std::string::npos) {
    if (wait > 0) Sleep(wait);

    // next time wait 100ms
    wait = 100;

    CurlHTTPRequest req(SERVER, false);

    StrPtr html = req.get(url);

    const Headers& headers = req.getHeaders();

    const std::string* cookieHeader = headers.find(COOKIE_HEADER);

    if (cookieHeader == 0) {
//      cookie = cookieHeader->substr(0, cookieHeader->find(";"));

      boost::smatch result;
      if (boost::regex_search(*html, result, CRUMB_REGEX)) {
        crumb = result[1];
        lastSuccessfulUpdate = yloader::LocalTimeSec();
        LOG(log_debug, _T("Successful yahoo token update, cookie: ")
                           << yloader::s2ws(cookie) << _T(", crumb: ")
                           << yloader::s2ws(crumb))
      } else {
        LOG(log_error, _T("Updating Yahoo token, could not get crumb"))
        throw std::exception("Could not get crumb");
      }
    } else {
      LOG(log_debug, _T("Updating Yahoo token, could not get cookie"))
      throw std::exception("Could not get cookie");
    }
  }
}
