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

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "BintrayClient.h"


BintrayClient::BintrayClient(const String &user, const String &repository, const String &package)
    : m_user(user), m_repo(repository), m_package(package),
      m_storage_host("pax.express"),
      m_api_host("pax.express")
{
}

String BintrayClient::getUser() const
{
    return m_user;
}

String BintrayClient::getRepository() const
{
    return m_repo;
}

String BintrayClient::getPackage() const
{
    return m_package;
}

String BintrayClient::getStorageHost() const
{
    return m_storage_host;
}

String BintrayClient::getApiHost() const
{
    return m_api_host;
}

String BintrayClient::getLatestVersionRequestUrl() const
{
    return String("https://") + getApiHost() + "/packages/" + getUser() + "/" + getRepository() + "/" + getPackage() + "/versions/_latest";
}

String BintrayClient::getBinaryRequestUrl(const String &version) const
{
    return String("https://") + getApiHost() + "/packages/" + getUser() + "/" + getRepository() + "/" + getPackage() + "/versions/" + version + "/files";
}

const char *BintrayClient::getCertificate(const String &url) const
{
    for(auto& cert: m_certificates) {
        if(url.indexOf(cert.first) >= 0) {
            return cert.second;
        }
    }

    // Return the certificate for *.bintray.com by default
    return m_certificates.rbegin()->second;
}

String BintrayClient::requestHTTPContent(const String &url) const
{
    String payload;
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
            payload = http.getString();
        }
    }
    else
    {
        ESP_LOGE(TAG, "GET request failed, error: %s", http.errorToString(httpCode).c_str());
    }

    http.end();
    return payload;
}

String BintrayClient::getLatestVersion() const
{
    String version;
    const String url = getLatestVersionRequestUrl();
    String jsonResult = requestHTTPContent(url);
    const size_t bufferSize = 1024;
    if (jsonResult.length() > bufferSize)
    {
        ESP_LOGE(TAG, "Error: Firmware version data invalid.");
        return version;
    }
    StaticJsonDocument<bufferSize> doc;

    DeserializationError err = deserializeJson(doc, jsonResult.c_str());

    // Check for errors in parsing
    if (err)
    {
        ESP_LOGE(TAG, "Error: Firmware version data not found.");
        return version;
    }
    return doc["name"].as<String>();
}

String BintrayClient::getBinaryPath(const String &version) const
{
    String path;
    const String url = getBinaryRequestUrl(version);
    String jsonResult = requestHTTPContent(url);

    const size_t bufferSize = 1024;
    if (jsonResult.length() > bufferSize)
    {
        ESP_LOGE(TAG, "Error: Firmware download path data invalid.");
        return path;
    }
    StaticJsonDocument<bufferSize> doc;

    DeserializationError err = deserializeJson(doc, jsonResult.c_str());

    JsonObject firstItem = doc[0];
    if (err)
    { //Check for errors in parsing
        ESP_LOGE(TAG, "Error: Firmware download path not found.");
        return path;
    }
    return "/" + getUser() + "/" + getRepository() + "/" + firstItem["path"].as<String>();
}
