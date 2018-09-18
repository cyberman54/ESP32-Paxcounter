# build.py
# pre-build script, setting up build environment

import requests
from os.path import basename
from platformio import util

Import("env")

# get keyfile from platformio.ini and parse it
project_config = util.load_project_config()
keyfile = str(env.get("PROJECTSRC_DIR")) + "/" + project_config.get("common", "keyfile")
print "Parsing OTA keys from " + keyfile
mykeys = {}
with open(keyfile) as myfile:
    for line in myfile:
        key, value = line.partition("=")[::2]
        mykeys[key.strip()] = str(value).strip()

# get bintray credentials from keyfile
user = mykeys["BINTRAY_USER"]
repository = mykeys["BINTRAY_REPO"]
apitoken = mykeys["BINTRAY_API_TOKEN"]

# get bintray parameters from platformio.ini
version = project_config.get("common", "release_version")
package = str(env.get("PIOENV"))

# put bintray credentials to platformio environment
env.Replace(BINTRAY_USER=user)
env.Replace(BINTRAY_REPO=repository)
env.Replace(BINTRAY_API_TOKEN=apitoken)

# get runtime credentials from keyfile and put them to compiler directive
env.Replace(CPPDEFINES=[
    ('WIFI_SSID', '\\"' + mykeys["OTA_WIFI_SSID"] + '\\"'), 
    ('WIFI_PASS', '\\"' + mykeys["OTA_WIFI_PASS"] + '\\"'),
    ('BINTRAY_USER', '\\"' + mykeys["BINTRAY_USER"] + '\\"'),
    ('BINTRAY_REPO', '\\"' + mykeys["BINTRAY_REPO"] + '\\"'),
    ])

# function for pushing new firmware to bintray storage using API
def publish_bintray(source, target, env):
    firmware_path = str(source[0])
    firmware_name = basename(firmware_path)
    url = "/".join([
        "https://api.bintray.com", "content",
        user, repository, package, version, firmware_name
    ])

    print("Uploading {0} to Bintray. Version: {1}".format(
        firmware_name, version))
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
        auth=(user, apitoken))

    if r.status_code != 201:
        print("Failed to submit package: {0}\n{1}".format(
            r.status_code, r.text))
    else:
        print("The firmware has been successfuly published at Bintray.com!")

# put build file name and upload command to platformio environment
env.Replace(
    PROGNAME="firmware_" + package + "_v%s" % version,
    UPLOADCMD=publish_bintray
)