#!/bin/bash

function clean_workspace ()
{
  echo "------------------------------------"
  echo "Cleaning workspace"
  echo "------------------------------------"

  rm -rf _build
  rm -rf linux64
  rm -rf *.so
  rm -f version.properties 
  echo -e "${FGRN}Complete${RS}"
  echo""
}

function find_version_number ()
{
  echo "------------------------------------"
  echo "Extracting Git Revision Number"
  echo "------------------------------------"

  SEMANTIC=`cat symantic.version`

  function trim() { echo $1; }

  if [ -z $GIT_HOME ]; then
    if [ -f "/usr/local/bin/git" ]; then
      export GIT_HOME=/usr/local/bin/git
    elif [ -f "/usr/bin/git" ]; then
      export GIT_HOME=/usr/bin/git
    elif [ -f "/bin/git" ]; then
      export GIT_HOME=/bin/git
    else
      ${FRED}GIT Could not be located${RS}
      EXIT_CODE=55
      failed_exit
    fi
  fi

  git status
  if [ $? == 0 ]; then
    export GIT_REV_COUNT_RAW=`$GIT_HOME log --pretty=oneline | wc -l`
    export GIT_REV_COUNT=`trim $GIT_REV_COUNT_RAW`
    export GIT_REV=`$GIT_HOME rev-parse HEAD`
    VERSION=$SEMANTIC.$TAG$GIT_REV_COUNT
  else
    export GIT_REV_COUNT='release'
    export GIT_REV='release'
    VERSION=$SEMANTIC.r
  fi
  echo "Version: $VERSION"
  echo "# THIS IS A GENERATED FILE " > version.properties
  echo "version='$VERSION'" >> version.properties
  echo "revision='$GIT_REV'" >> version.properties
  echo "#define PEACHY_USB_VERSION \"$VERSION\"" > inc/version.h
  echo "Git Revision Number is $GIT_REV_COUNT"
  echo ""
}

function build_so () {
  echo "------------------------------------"
  echo "Build so"
  echo "------------------------------------"

  mkdir linux64
  if [ $? != 0 ]; then
    echo "${FRED}FAILURE: making directory linux64${RS}"
    exit 101
  fi
  mkdir _build
  if [ $? != 0 ]; then
    echo "${FRED}FAILURE: making directory _build${RS}"
    exit 101
  fi
  cd _build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  if [ $? != 0 ]; then
    echo "${FRED}FAILURE: cmake ..${RS}"
    exit 102
  fi
  make
  if [ $? != 0 ]; then
    echo "${FRED}FAILURE: make${RS}"
    exit 103
  fi
  cp src/libPeachyUSB.so ../linux64
  if [ $? != 0 ]; then
    echo "${FRED}FAILURE: cp src/libPeachyUSB.so ..${RS}"
    exit 104
  fi
}

clean_workspace
find_version_number
build_so
