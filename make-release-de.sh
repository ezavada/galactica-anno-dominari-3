#!/bin/bash

CPMAC="/Developer/Tools/CpMac -p"
#CPMAC="echo /Developer/Tools/CpMac -p"

# application version number

VERSION=3.0
LANGUAGE=de

################################
# move stuff into CD-ROM release
################################

### OS X ###

TARGET_DIR=release/cdrom
TARGET_BIN_DIR=$TARGET_DIR/Galactica.app/Contents/MacOS

echo $TARGET_BIN_DIR

echo "Building OS X CD-ROM release in $TARGET_DIR"

#base copy from build directory
rm -rf $TARGET_DIR/Galactica.app
cp -Rf build/Galactica.app $TARGET_DIR
rm -rf $TARGET_BIN_DIR/Plug-ins
rm -f "$TARGET_BIN_DIR/Galactica Art (Large)"
rm -f "$TARGET_BIN_DIR/Galactica Music"

$CPMAC -r build/Galactica.app/Contents/MacOS/Plug-ins $TARGET_BIN_DIR
$CPMAC "Plug-ins/Galactica:$LANGUAGE - cdrom" $TARGET_BIN_DIR/Plug-ins/
$CPMAC "Galactica Art (Large)" $TARGET_BIN_DIR/
$CPMAC "Galactica Music" $TARGET_BIN_DIR/

#remove english specific stuff
rm -f "$TARGET_BIN_DIR/Plug-ins/Galactica Tutorial"
rm -f "$TARGET_BIN_DIR/Plug-ins/docs.html
rm -rf $TARGET_BIN_DIR/Plug-ins/images/

#add language specific stuff


################################
# move stuff into shareware release
################################

### OS X ###

TARGET_DIR=release/shareware
TARGET_BIN_DIR=$TARGET_DIR/Galactica.app/Contents/MacOS

echo $TARGET_BIN_DIR

echo "Building OS X CD-ROM release in $TARGET_DIR"

#base copy from build directory
rm -rf $TARGET_DIR/Galactica.app
cp -Rf build/Galactica.app $TARGET_DIR
rm -rf $TARGET_BIN_DIR/Plug-ins
rm -f "$TARGET_BIN_DIR/Galactica Art (Large)"
rm -f "$TARGET_BIN_DIR/Galactica Music"

$CPMAC -r build/Galactica.app/Contents/MacOS/Plug-ins $TARGET_BIN_DIR
$CPMAC "Plug-ins/Galactica:$LANGUAGE - shareware" $TARGET_BIN_DIR/Plug-ins/
$CPMAC "Galactica Art" $TARGET_BIN_DIR/
$CPMAC "Galactica Music" $TARGET_BIN_DIR/





#!/bin/bash

CPMAC="/Developer/Tools/CpMac -p"
#CPMAC="echo /Developer/Tools/CpMac -p"

RES_MERGE="/Developer/Tools/ResMerger -srcIs RSRC -dstIs RSRC"
#RES_MERGE="echo /Developer/Tools/ResMerger -srcIs RSRC -dstIs RSRC"

# application version number

VERSION=`cat release/bits/version.txt`
LANG_CODE=de
LANG_NAME=German

LANG=-$LANG_CODE

SRC_TUTORIAL_NAME="Galactica Anleitung"

DST_APP_NAME=Galactica
DST_TUTORIAL_NAME="Galactica Anleitung"
DST_ART_NAME="Galactica Art"
DST_ART_LARGE_NAME="Galactica Art (Large)"
DST_MUSIC_NAME="Galactica Music"

################################
# stamp version number into files
################################

echo "Backing up files to be modified into release/backup"

mkdir release
mkdir release/backup

$CPMAC "Plug-ins/Galactica Fonts" release/backup/
$CPMAC "Plug-ins/$SRC_TUTORIAL_NAME" release/backup/
$CPMAC "Plug-ins/Galactica:$LANG_CODE - cdrom" release/backup/
$CPMAC "Plug-ins/Galactica:$LANG_CODE - shareware" release/backup/
$CPMAC Galactica release/backup/
$CPMAC "Galactica Art" release/backup/
$CPMAC "Galactica Art (Large)" release/backup/
$CPMAC "Galactica Music" release/backup/


echo "Stamping files with version number $VERSION"


$CPMAC "Plug-ins/Galactica Fonts" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Plug-ins/Galactica Fonts"
rm tmpfile

$CPMAC "Plug-ins/$SRC_TUTORIAL_NAME" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Plug-ins/$SRC_TUTORIAL_NAME"
rm tmpfile

$CPMAC "Plug-ins/Galactica:$LANG_CODE - cdrom" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Plug-ins/Galactica:$LANG_CODE - cdrom"
rm tmpfile

$CPMAC "Plug-ins/Galactica:$LANG_CODE - shareware" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Plug-ins/Galactica:$LANG_CODE - shareware"
rm tmpfile

$CPMAC Galactica tmpfile
$RES_MERGE release/bits/$LANG_CODE/version release/bits/$LANG_CODE/plist.rsrc tmpfile -o "Galactica"
rm tmpfile

$CPMAC "Galactica Art" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Galactica Art"
rm tmpfile

$CPMAC "Galactica Art (Large)" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Galactica Art (Large)"
rm tmpfile

$CPMAC "Galactica Music" tmpfile
$RES_MERGE release/bits/$LANG_CODE/version tmpfile -o "Galactica Music"
rm tmpfile



################################
# make directories
################################

mkdir release
mkdir release/cdrom$LANG
mkdir release/cdrom$LANG/$DST_APP_NAME$LANG.app
mkdir release/cdrom$LANG/$DST_APP_NAME$LANG.app/Contents
mkdir release/cdrom$LANG/$DST_APP_NAME$LANG.app/Contents/MacOS
mkdir release/cdrom$LANG/$DST_APP_NAME$LANG.app/Contents/MacOS/Plug-ins
mkdir release/cdrom$LANG/$DST_APP_NAME$LANG.app/Contents/Resources
mkdir release/cdrom$LANG/$DST_APP_NAME-$VERSION$LANG/
mkdir release/cdrom$LANG/$DST_APP_NAME-$VERSION$LANG/Plug-ins

mkdir release
mkdir release/shareware$LANG
mkdir release/shareware$LANG/$DST_APP_NAME$LANG.app
mkdir release/shareware$LANG/$DST_APP_NAME$LANG.app/Contents
mkdir release/shareware$LANG/$DST_APP_NAME$LANG.app/Contents/MacOS
mkdir release/shareware$LANG/$DST_APP_NAME$LANG.app/Contents/MacOS/Plug-ins
mkdir release/shareware$LANG/$DST_APP_NAME$LANG.app/Contents/Resources
mkdir release/shareware$LANG/$DST_APP_NAME-$VERSION$LANG/
mkdir release/shareware$LANG/$DST_APP_NAME-$VERSION$LANG/Plug-ins


################################
# move stuff into CD-ROM release
################################

### OS X ###

TARGET_DIR=release/cdrom$LANG/$DST_APP_NAME$LANG.app/Contents/MacOS

echo "Building OS X $LANG_NAME CD-ROM release in $TARGET_DIR"

#documentation
$CPMAC Plug-ins/docs$LANG.html $TARGET_DIR/Plug-ins/docs.html
$CPMAC -r "Plug-ins/images$LANG" $TARGET_DIR/Plug-ins/images

#language stuff
$CPMAC "Plug-ins/Galactica Fonts" $TARGET_DIR/Plug-ins/
$CPMAC "Plug-ins/$SRC_TUTORIAL_NAME" "$TARGET_DIR/Plug-ins/$DST_TUTORIAL_NAME"
$CPMAC "Plug-ins/Galactica:$LANG_CODE - cdrom" "$TARGET_DIR/Plug-ins/$DST_APP_NAME:$LANG_CODE - cdrom"

#App, art and music
$CPMAC Galactica "$TARGET_DIR/$DST_APP_NAME"
$CPMAC "Galactica Art (Large)" "$TARGET_DIR/$DST_ART_LARGE_NAME"
$CPMAC "Galactica Music" "$TARGET_DIR/$DST_MUSIC_NAME"

#Support stuff
$CPMAC release/bits/OpenPlayLib $TARGET_DIR/
$CPMAC -r "release/bits/OpenPlay Modules" $TARGET_DIR/

# package stuff
$CPMAC release/bits/Galactica.icns $TARGET_DIR/../Resources/
$CPMAC release/bits/PkgInfo $TARGET_DIR/../
$CPMAC release/bits/$LANG_CODE/Info.plist $TARGET_DIR/../

### OS 9 ###

TARGET_DIR=release/cdrom$LANG/$DST_APP_NAME-$VERSION$LANG

echo "Building OS 9 $LANG_NAME CD-ROM release in $TARGET_DIR"

#documentation
$CPMAC Plug-ins/docs$LANG.html "$TARGET_DIR"/Plug-ins/docs.html
$CPMAC -r "Plug-ins/images$LANG" $TARGET_DIR/Plug-ins/images

#language stuff
$CPMAC "Plug-ins/Galactica Fonts" $TARGET_DIR/Plug-ins/
$CPMAC "Plug-ins/$SRC_TUTORIAL_NAME" "$TARGET_DIR/Plug-ins/$DST_TUTORIAL_NAME"
$CPMAC "Plug-ins/Galactica:$LANG_CODE - cdrom" "$TARGET_DIR/Plug-ins/$DST_APP_NAME:$LANG_CODE - cdrom"

#App, art and music
$CPMAC Galactica "$TARGET_DIR/$DST_APP_NAME"
$CPMAC "Galactica Art (Large)" "$TARGET_DIR/$DST_ART_LARGE_NAME"
$CPMAC "Galactica Music" "$TARGET_DIR/$DST_MUSIC_NAME"

#Support stuff
$CPMAC release/bits/OpenPlayLib $TARGET_DIR/
$CPMAC -r "release/bits/OpenPlay Modules" $TARGET_DIR/


################################
# move stuff into shareware release
################################

### OS X ###

TARGET_DIR=release/shareware$LANG/$DST_APP_NAME$LANG.app/Contents/MacOS

echo "Building OS X $LANG_NAME Shareware release in $TARGET_DIR"

#documentation
$CPMAC Plug-ins/docs$LANG.html $TARGET_DIR/Plug-ins/docs.html
$CPMAC -r "Plug-ins/images$LANG" $TARGET_DIR/Plug-ins/images

#language stuff
$CPMAC "Plug-ins/Galactica Fonts" $TARGET_DIR/Plug-ins/
$CPMAC "Plug-ins/$SRC_TUTORIAL_NAME" "$TARGET_DIR/Plug-ins/$DST_TUTORIAL_NAME"
$CPMAC "Plug-ins/Galactica:$LANG_CODE - shareware" "$TARGET_DIR/Plug-ins/$DST_APP_NAME:$LANG_CODE - shareware"

#App, art and music
$CPMAC Galactica "$TARGET_DIR/$DST_APP_NAME"
$CPMAC "Galactica Art" "$TARGET_DIR/$DST_ART_NAME"
$CPMAC "Galactica Music" "$TARGET_DIR/$DST_MUSIC_NAME"

#Support stuff
$CPMAC release/bits/OpenPlayLib $TARGET_DIR/
$CPMAC -r "release/bits/OpenPlay Modules" $TARGET_DIR/

# package stuff
$CPMAC release/bits/Galactica.icns $TARGET_DIR/../Resources/
$CPMAC release/bits/PkgInfo $TARGET_DIR/../
$CPMAC release/bits/$LANG_CODE/Info.plist $TARGET_DIR/../


### OS 9 ###

TARGET_DIR="release/shareware$LANG/$DST_APP_NAME-$VERSION$LANG"

echo "Building OS 9 $LANG_NAME Shareware release in $TARGET_DIR"

#documentation
$CPMAC Plug-ins/docs$LANG.html $TARGET_DIR/Plug-ins/docs.html
$CPMAC -r "Plug-ins/images$LANG" $TARGET_DIR/Plug-ins/images

#language stuff
$CPMAC "Plug-ins/Galactica Fonts" $TARGET_DIR/Plug-ins/
$CPMAC "Plug-ins/$SRC_TUTORIAL_NAME" "$TARGET_DIR/Plug-ins/$DST_TUTORIAL_NAME"
$CPMAC "Plug-ins/Galactica:$LANG_CODE - shareware" "$TARGET_DIR/Plug-ins/$DST_APP_NAME:$LANG_CODE - shareware"

#App, art and music
$CPMAC Galactica "$TARGET_DIR/$DST_APP_NAME"
$CPMAC "Galactica Art" "$TARGET_DIR/$DST_ART_NAME"
$CPMAC "Galactica Music" "$TARGET_DIR/$DST_MUSIC_NAME"

#Support stuff
$CPMAC release/bits/OpenPlayLib $TARGET_DIR/
$CPMAC -r "release/bits/OpenPlay Modules" $TARGET_DIR/

