/*
 Parts of this file
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/

#ifndef BINTRAY_CLIENT_H
#define BINTRAY_CLIENT_H

#include <vector>
#include <utility>
#include <WString.h>

class BintrayClient {

public:
    BintrayClient(const String& user, const String& repository, const String& package);
    String getUser() const;
    String getRepository() const;
    String getPackage() const;
    String getStorageHost() const;
    String getApiHost() const;
    const char* getCertificate(const String& url) const;
    String getLatestVersion() const;
    String getBinaryPath(const String& version) const;

private:
    String requestHTTPContent(const String& url) const;
    String getLatestVersionRequestUrl() const;
    String getBinaryRequestUrl(const String& version) const;
    String m_user;
    String m_repo;
    String m_package;
    const String m_storage_host;
    const String m_api_host;
    std::vector<std::pair<String, const char*>> m_certificates;
};

#endif // BINTRAY_CLIENT_H
