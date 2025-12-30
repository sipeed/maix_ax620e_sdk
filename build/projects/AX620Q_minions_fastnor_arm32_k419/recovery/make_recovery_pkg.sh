#!/usr/bin/env bash

set -e

PWD=`pwd`

PROJECT=$1
OTA_PKG=${PROJECT}_recovery_$(date "+%Y%m%d%H%M%S").swu

echo "PROJECT=${PROJECT}"
OUT_PATH=${PWD}/../../out/${PROJECT}/images

cp -rf ${PWD}/recovery/mkswu/recovery.sh ${OUT_PATH}
cp -rf ${PWD}/recovery/mkswu/swugenerator.sh ${OUT_PATH}
cp -rf ${PWD}/recovery/mkswu/sw-description-source ${OUT_PATH}
#cp -rf ${PWD}/recovery/mkswu/priv.pem ${OUT_PATH}

cd ${OUT_PATH}
sh swugenerator.sh ${OTA_PKG}
mv ${OTA_PKG} ../../
rm -rf swugenerator.sh
rm -rf recovery.sh
#rm -rf priv.pem
rm -rf sw-description-source
cd -

