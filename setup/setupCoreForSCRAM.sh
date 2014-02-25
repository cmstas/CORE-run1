#!/bin/bash

# ---------------------------------------------------------------------------------- #
# Simple setup script to create the CMS2.h/cc files and setup symlinks to compile with
# CMSSW's scram and runs with cmsRun.  There is another script to use if you with to 
# run with SCRAM without using cmsRun called setupCoreForSCRAM.sh.
# It uses the makeTTreeClassFiles.py
#
# Usage: setupCoreForCMSSW.sh ntuple_file_name
# ---------------------------------------------------------------------------------- #

# CMSSW must be set
if [ -z $CMSSW_BASE ]; then
    echo "[setupCoreForCMSSW.sh] Error: CMSSW must be set"
    exit 1
fi

# must pass a vaild cms2 ntuple file
if [ $# -ne 1 ]; then
    echo "[setupCoreForCMSSW.sh] Usage: setupCoreForCMSSW.sh ntuple_file_name"
    exit 1
fi

# setup CORE symlinks
# ------------------------------------------------------------- #

echo "[setupCoreForCMSSW.sh] creating symlinks"

core_dir="$CMSSW_BASE/src/CMS2/NtupleMacrosCore"

pushd $core_dir

mkdir -p $core_dir/interface
mkdir -p $core_dir/interface/dummy_dir # needed for #include "../CMS2.h" :(
mkdir -p $core_dir/src

function create_header_ln
{
    local file=$1
    pushd interface
    ln -sf ../$file
    popd
}

function create_source_ln
{
    local file=$1
    pushd src
    ln -sf ../$file
    popd
}

# sym link header files
create_header_ln EventShape.h
create_header_ln MITConversionUtilities.h
create_header_ln SimpleFakeRate.h
create_header_ln Thrust.h
create_header_ln conversionTools.h
create_header_ln electronSelections.h
create_header_ln electronSelectionsParameters.h
create_header_ln mcSUSYkfactor.h
create_header_ln mcbtagSFuncert.h
create_header_ln susySelections.h
create_header_ln eventSelections.h
create_header_ln muonSelections.h
create_header_ln mcSelections.h
create_header_ln trackSelections.h
create_header_ln triggerSuperModel.h
create_header_ln triggerUtils.h
create_header_ln utilities.h
create_header_ln jetSelections.h
create_header_ln metSelections.h
create_header_ln ttbarSelections.h
create_header_ln photonSelections.h
create_header_ln jetSmearingTools.h
create_header_ln osSelections.h
create_header_ln ttvSelections.h
create_header_ln ssSelections.h
create_header_ln JetMETUncertainty.h
create_header_ln jetcorr
create_header_ln jetsmear
create_header_ln tcmet
create_header_ln MT2
create_header_ln QuarkGluonTagger

# sym link source files
create_source_ln EventShape.cc
create_source_ln MITConversionUtilities.cc
create_source_ln SimpleFakeRate.cc
create_source_ln Thrust.cc
create_source_ln conversionTools.cc
create_source_ln electronSelections.cc
create_source_ln electronSelectionsParameters.cc
create_source_ln mcSUSYkfactor.cc
create_source_ln mcbtagSFuncert.cc
create_source_ln susySelections.cc
create_source_ln eventSelections.cc
create_source_ln muonSelections.cc
create_source_ln mcSelections.cc
create_source_ln trackSelections.cc
create_source_ln triggerSuperModel.cc
create_source_ln triggerUtils.cc
create_source_ln utilities.cc
create_source_ln jetSelections.cc
create_source_ln metSelections.cc
create_source_ln photonSelections.cc
create_source_ln osSelections.cc
create_source_ln ttbarSelections.cc
create_source_ln ttvSelections.cc
create_source_ln ssSelections.cc
create_source_ln JetMETUncertainty.cc
create_source_ln MT2/MT2.cc
create_source_ln MT2/MT2Utility.cc
create_source_ln QuarkGluonTagger/QuarkGluonTagger.cc
create_source_ln QuarkGluonTagger/QGLikelihoodCalculator.cc
create_source_ln jetsmear/JetResolution.cc
create_source_ln jetsmear/JetSmearer.cc
create_source_ln jetsmear/SigInputObj.cc
create_source_ln jetSmearingTools.cc

popd

echo "<flags CPPDEFINES=\"CMS2_USE_CMSSW=1\"/>
<use name=\"CMS2/NtupleMacrosHeader\"/>
<use name=\"rootrflx\"/>
<use name=\"roofit\"/>
<use name=\"rootcore\"/>
<use name=\"root\"/>
<lib name=\"EG\"/>
<export>
  <use name=\"CMS2/NtupleMacrosHeader\"/>
  <use name=\"root\"/>
  <lib name=\"1\"/>
</export>" > $core_dir/BuildFile.xml

# setup CMS2.h 
# ------------------------------------------------------------- #

echo "[setupCoreForCMSSW.sh] creating CMS2.h/cc"

header_dir=$CMSSW_BASE/src/CMS2/NtupleMacrosHeader
file_name=$1
mkdir -p $header_dir
mkdir -p $header_dir/interface/dummy_dir # needed for #include "../CMS2.h" :(

# create the CMS2.cc/h
cmd="root -b -q -l \"$core_dir/setup/makeCMS2ClassFiles.C+(\\\"$file_name\\\", false, \\\"Events\\\", \\\"\\\", \\\"CMS2\\\", \\\"tas\\\", \\\"cms2\\\")\"" 
echo $cmd
eval $cmd

# move CMS2.h
echo "[setupCoreForCMSSW.sh] moving CMS2.h to $header_dir/interface"
mkdir -p $header_dir/interface
mv CMS2.h $header_dir/interface/.

# move CMS2.cc
echo "[setupCoreForCMSSW.sh] moving CMS.cc to $header_dir/src"
mkdir -p $header_dir/src
sed 's/\#include "CMS2.h"/\#include "CMS2\/NtupleMacrosHeader\/interface\/CMS2.h"/g' CMS2.cc > $header_dir/src/CMS2.cc
rm $core_dir/CMS2.cc

# remove example files
rm $core_dir/doAll.C
rm $core_dir/ScanChain.C

# setup BuildFile.xml
echo "[setupCoreForCMSSW.sh] setting up  $header_dir/BuildFile.xml"
echo "<use name=\"root\"/>
<export>
  <use name=\"root\"/>
  <lib name=\"1\"/>
</export>" > $header_dir/BuildFile.xml

# compile 
# ------------------------------------------------------------- #

echo "[setupCoreForCMSSW.sh] compiling"
pushd $core_dir
scram b -j20
popd

# done
# ------------------------------------------------------------- #
echo "[setupCoreForCMSSW.sh] finished"

