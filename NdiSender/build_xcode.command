#!/bin/sh

echo '--- Define script directory ---'
SCRIPT_DIRECTORY=$(cd $(dirname $0);pwd) 
cd ${SCRIPT_DIRECTORY}

# Script job will terminate when error occured.
set -e

echo '--- Set variables ---'
PROJECT_NAME=NdiSender
ARCHITECTURE=x86_64
BUILD_CONFIG=Release
EXPORTER_NAME=MacOSX

echo '--- Generate IDE project file by Projucer ---'
${SCRIPT_DIRECTORY}/../Projucer/Projucer.app/Contents/MacOS/Projucer --resave ${SCRIPT_DIRECTORY}/${PROJECT_NAME}.jucer

echo '--- Get solution file name from Projucer ---'
SOLUTION_NAME=`${SCRIPT_DIRECTORY}/../Projucer/Projucer.app/Contents/MacOS/Projucer --status ${SCRIPT_DIRECTORY}/${PROJECT_NAME}.jucer | grep "Name:" | awk '{ print $2 }'`

echo '--- Get project version number from Projucer ---'
VERSION_NUMBER=`${SCRIPT_DIRECTORY}/../Projucer/Projucer.app/Contents/MacOS/Projucer --get-version ${SCRIPT_DIRECTORY}/${PROJECT_NAME}.jucer`

echo '--- Show variables ---'
echo 'SCRIPT_DIRECTORY: '${SCRIPT_DIRECTORY}
echo 'PROJECT_NAME: '${PROJECT_NAME}
echo 'VERSION_NUMBER:'${VERSION_NUMBER}
echo 'ARCHITECTURE: '${ARCHITECTURE}
echo 'BUILD_CONFIG: '${BUILD_CONFIG}
echo 'SOLUTION_NAME: '${SOLUTION_NAME}
echo 'EXPORTER_NAME: '${EXPORTER_NAME}

echo '--- Show list of '${SOLUTION_NAME}'.xcodeproj ---'
xcodebuild -project "${SCRIPT_DIRECTORY}/Builds/${EXPORTER_NAME}/${SOLUTION_NAME}.xcodeproj" -list

echo '--- Show Xcode build settings ---'
xcodebuild -project "${SCRIPT_DIRECTORY}/Builds/${EXPORTER_NAME}/${SOLUTION_NAME}.xcodeproj" \
-alltargets \
-configuration ${BUILD_CONFIG} \
-arch ${ARCHITECTURE} \
-showBuildSettings

echo '--- Run Xcode build ---'
xcodebuild -project "${SCRIPT_DIRECTORY}/Builds/${EXPORTER_NAME}/${SOLUTION_NAME}.xcodeproj" \
-alltargets \
-configuration ${BUILD_CONFIG} \
-arch ${ARCHITECTURE}

echo '--- Rename to adding version number for VST3 file --'
SRC_FILE=${SCRIPT_DIRECTORY}/Builds/${EXPORTER_NAME}/build/${BUILD_CONFIG}/${SOLUTION_NAME}.vst3
DEST_FILE=${SCRIPT_DIRECTORY}/Builds/${EXPORTER_NAME}/build/${BUILD_CONFIG}/${SOLUTION_NAME}-${VERSION_NUMBER}.vst3
if test -e `${DEST_FILE}`; then
    rm -rf ${DEST_FILE}
fi
if test -e `${SRC_FILE}`; then
    mv -f ${SRC_FILE} ${DEST_FILE}
fi
