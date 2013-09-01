#!/bin/bash

# Where to install Valgrind with ThreadSanitizer.
VALGRIND_INST_ROOT="$1"
SVN_ROOT="$2"

if [ "$VALGRIND_INST_ROOT" == "" ]; then
  echo "Usage: $0 /tsan/installation/path [svn/root/dir]"
  exit
fi

if [ "$SVN_ROOT" == "" ]; then
# Get ThreadSanitizer. This will create directory 'drt'
  svn co http://data-race-test.googlecode.com/svn/trunk drt || exit 1
  cd drt || exit 1
else
  cd $SVN_ROOT || exit 1
fi

TOPDIR=`pwd`

VG_ARCH=$(uname -m | sed -e "s/i.86/x86/;s/x86_64/amd64/;s/arm.*/arm/")

# Translate OS to valgrind-style identifiers
OS=`uname -s`
if [ "$OS" == "Linux" ]; then
  VG_OS="linux"
elif [ "$OS" == "Darwin" ]; then
  VG_OS="darwin"
fi

if ! echo -n "$OS $VG_ARCH" | \
     grep "\(Linux \(amd64\|x86\)\)\|Darwin x86" >/dev/null
then
  echo "ThreadSanitizer is not yet supported on $OS $VG_ARCH"
  exit 1
fi

echo ------------------------------------------------
echo Building ThreadSanitizer for $OS $VG_ARCH
echo ------------------------------------------------
sleep 1

# Build Valgind.
cd $TOPDIR/third_party || exit 1
./update_valgrind.sh || exit 1
./build_and_install_valgrind.sh $VALGRIND_INST_ROOT || exit 1

cd $TOPDIR/tsan || exit 1
make -s -j4 OFFLINE= GTEST_ROOT= PIN_ROOT= VALGRIND_INST_ROOT=$VALGRIND_INST_ROOT || exit 1
# Build the self contained binaries.
make self-contained OS=$VG_OS ARCH=$VG_ARCH VALGRIND_INST_ROOT=$VALGRIND_INST_ROOT || exit 1

TSAN=$TOPDIR/tsan/bin/tsan-$VG_ARCH-$VG_OS-self-contained.sh

# Test
cd $TOPDIR/unittest || exit 1
make all -s -j4 OS=${VG_OS} ARCH=${VG_ARCH} OPT=1 STATIC=0 || exit 1
$TSAN --color bin/demo_tests-${VG_OS}-${VG_ARCH}-O1 --gtest_filter="DemoTests.RaceReportDemoTest" || exit 1

# Done
echo "ThreadSanitizer is built: $TSAN"
