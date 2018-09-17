# Copyright (c) 2014-present PlatformIO <contact@platformio.org>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import requests
from os.path import basename
from platformio import util

Import("env")

project_config = util.load_project_config()
bintray_config = {k: v for k, v in project_config.items("bintray")}
version = project_config.get("ota", "release_version")
package = env.get("PIOENV")

#
# Push new firmware to the Bintray storage using API
#


def publish_bintray(source, target, env):
    firmware_path = str(source[0])
    firmware_name = basename(firmware_path)

    print("Uploading {0} to Bintray. Version: {1}".format(
        firmware_name, version))

    url = "/".join([
        "https://api.bintray.com", "content",
        bintray_config.get("user"),
        bintray_config.get("repository"),
        package, version, firmware_name
    ])

    print(url)

    headers = {
        "Content-type": "application/octet-stream",
        "X-Bintray-Publish": "1",
        "X-Bintray-Override": "1"
    }

    r = requests.put(
        url,
        data=open(firmware_path, "rb"),
        headers=headers,
        auth=(bintray_config.get("user"), bintray_config['api_token']))

    if r.status_code != 201:
        print("Failed to submit package: {0}\n{1}".format(
            r.status_code, r.text))
    else:
        print("The firmware has been successfuly published at Bintray.com!")


# Custom upload command and program name

env.Replace(
    PROGNAME="firmware_" + package + "_v%s" % version,
    UPLOADCMD=publish_bintray
)