#!/bin/bash

CPMAC="/Developer/Tools/CpMac -p"
#CPMAC="echo /Developer/Tools/CpMac -p"

# application version number

VERSION=3.0.2

################################
# move stuff into CD-ROM release
################################

### OS X ###

TARGET_DIR=release/cdrom
TARGET_BIN_DIR=$TARGET_DIR/Galactica.app/Contents/MacOS

SOURCE_DIR=build/Deployment_Box_Version

echo $TARGET_BIN_DIR

echo "Building OS X CD-ROM release in $TARGET_DIR"

#base copy from build directory
rm -rf $TARGET_DIR/Galactica.app
cp -Rf $SOURCE_DIR/Galactica.app $TARGET_DIR
rm -rf $TARGET_BIN_DIR/Plug-ins
rm -f "$TARGET_BIN_DIR/Galactica Art (Large)"
rm -f "$TARGET_BIN_DIR/Galactica Music"

$CPMAC -r $SOURCE_DIR/Galactica.app/Contents/MacOS/Plug-ins $TARGET_BIN_DIR
$CPMAC "Plug-ins/Galactica:en - cdrom" $TARGET_BIN_DIR/Plug-ins/
rm -f  "$TARGET_BIN_DIR/Plug-ins/Galactica:en - shareware"
$CPMAC "Galactica Art (Large)" $TARGET_BIN_DIR/
$CPMAC "Galactica Music" $TARGET_BIN_DIR/

################################
# move stuff into shareware release
################################

### OS X ###

TARGET_DIR=release/shareware
TARGET_BIN_DIR=$TARGET_DIR/Galactica.app/Contents/MacOS

SOURCE_DIR=build/Deployment

echo $TARGET_BIN_DIR

echo "Building OS X Shareware release in $TARGET_DIR"

#base copy from build directory
rm -rf $TARGET_DIR/Galactica.app
cp -Rf $SOURCE_DIR/Galactica.app $TARGET_DIR
rm -rf $TARGET_BIN_DIR/Plug-ins
rm -f "$TARGET_BIN_DIR/Galactica Art (Large)"
rm -f "$TARGET_BIN_DIR/Galactica Music"

$CPMAC -r $SOURCE_DIR/Galactica.app/Contents/MacOS/Plug-ins $TARGET_BIN_DIR
$CPMAC "Plug-ins/Galactica:en - shareware" $TARGET_BIN_DIR/Plug-ins/
rm -f  "$TARGET_BIN_DIR/Plug-ins/Galactica:en - cdrom"
$CPMAC "Galactica Art" $TARGET_BIN_DIR/
$CPMAC "Galactica Music" $TARGET_BIN_DIR/

