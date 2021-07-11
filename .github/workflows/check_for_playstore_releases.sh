curl_parameters=("-b" "cookie_jar" "-c" "cookie_jar" "-H" "authority: ${APPSTORE_HOST}" "-H" "pragma: no-cache" "-H" "cache-control: no-cache" "-H" "dnt: 1" "-H" "upgrade-insecure-requests: 1" "-H" "user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML like Gecko) Chrome/81.0.4044.113 Safari/537.36" "-H" "accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9" "-H" "sec-fetch-site: none" "-H" "sec-fetch-mode: navigate" "-H" "sec-fetch-dest: document" "-H" "content-type: application/json" "-H" "accept-language: en-US,en;q=0.9" "--connect-timeout" "5" "--max-time" "10" "--retry" "5" "--retry-delay" "0" "--retry-max-time" "40" "--compressed")
proxy_parameters=("--proxy" "http://$PROXY_USER@$PROXY_HOST" "--user" "$AUTHORIZATION")
source_data=`curl -s -X POST "https://${APPSTORE_HOST}/${APPSTORE_LIST_PATH}" "${proxy_parameters[@]}" "${curl_parameters[@]}" -d '{ "pnames": ["com.kiwibrowser.browser"] }'`

versions_and_links=`echo $source_data | jq '.data[0].apks[].version_code, .data[0].apks[].link'`
if [ -z "$versions_and_links" ]
then
  echo "Invalid versions and links"
  exit -1
fi  
versions_and_links_hash=`echo $versions_and_links | shasum | cut -f1 -d' '`
links=`echo $source_data | jq '.data[0].apks[].link' | tr -d '"'`

# We use apksigner with v3 signature verification
# Ubuntu provides only version 0.8, so we take Android SDK Build Tools version 30
# apksigner 0.9

# Reference is:
########################################################################################################
# Signer #1 certificate DN: CN=Android, OU=Android, O=Google Inc., L=Mountain View, ST=California, C=US
# Signer #1 certificate SHA-256 digest: c069ea966332e91e0a0c3cc577e6f509fe3d9ddaf7015ad0c5c5ef8fda3f8628
# Signer #1 certificate SHA-1 digest: 42abd0c139a18ebddb75a724fba393fec443310f
# Signer #1 certificate MD5 digest: 6da12ea7b31fe810615e677fc73cd1cc
########################################################################################################
# All this block of text is equal to shasum 25f1bd4714cc90a293ab01e603f5ec7d6d7b2e1b

echo "Last version from App Store: $versions_and_links_hash"
for link in $links
do
  echo "Working on a link..."
  download=`curl -s "https://${APPSTORE_HOST}$link" "${proxy_parameters[@]}" "${curl_parameters[@]}"`
  download_id=`echo $download | egrep -o "<link rel='shortlink' href='\/\?p=([0-9]+)' \/>" | egrep -o '[0-9]+'`

  echo "Downloading target $download_id..."
  curl -C - -L "https://${APPSTORE_HOST}/${APPSTORE_DOWNLOAD_PATH}$download_id" "${curl_parameters[@]}" -o $download_id.apk

  signature=`java -jar apksigner.jar verify --print-certs $download_id.apk`
  signature_hash=`java -jar apksigner.jar verify --print-certs $download_id.apk | shasum | cut -f1 -d' '`

  echo "Signature: $signature"

  if [ "$signature_hash" != "25f1bd4714cc90a293ab01e603f5ec7d6d7b2e1b" ]
  then
    echo "Invalid signature: $signature_hash"
    exit -1
  fi

  echo "Getting platform for APK: $download_id"
  platform=`unzip -vl $download_id.apk | grep -Fi libchrome.so | cut -f2 -d'/'`
  if [ -z "$platform" ]
  then
    echo "Cannot determine platform"
    exit -1
  fi

  short_platform_name=""
  case "$platform" in
    "armeabi-v7a")
      short_platform_name="arm"
      ;;
    "arm64-v8a")
      short_platform_name="arm64"
      ;;
    "x86")
      short_platform_name="x86"
      ;;
    "x86_64")
      short_platform_name="x64"
      ;;
  esac
  if [ -z "$short_platform_name" ]
  then
    echo "Cannot determine short platform name"
    exit -1
  fi
  echo "Short platform name detected: $short_platform_name"
  filename="Kiwi-$short_platform_name-playstore.apk"
  mv -v "$download_id.apk" $filename
  sleep 1
done
