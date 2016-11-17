# speech engine lib
cd ./build/linux
make -f Makefile

# wake on voice demo
cd -
cd ./example/build/linux/wov
make -f Makefile

# speaker verification demo
cd -
cd ./example/build/linux/sv
make -f Makefile

# test bench
cd -
cd ./example/build/linux/test
make -f Makefile

# model training tool
cd -
cd ./tools/model_training
make -f Makefile

# score normalization tool
cd -
cd ./tools/score_normalization
make -f Makefile

cd -

failnum=0

# check whether build success
if [ -f ./lib/libaudiorecog.a ]
then
    echo " libaudiorecog.a generated"
else
    echo -e "\033[31m libaudiorecog.a FAILED \033[0m"
    failnum=`expr $failnum + 1`
fi

if [ -f ./bin/SpeakerVerifier ]
then
    echo " SpeakerVerifier generated"
else
    echo -e "\033[31m SpeakerVerifier FAILED \033[0m"
    failnum=`expr $failnum + 1`
fi

if [ -f ./bin/WakeonVoice ]
then
    echo " WakeonVoice generated"
else
    echo -e "\033[31m WakeonVoice FAILED \033[0m"
    failnum=`expr $failnum + 1`
fi

if [ -f ./bin/TestBench ]
then
    echo " TestBench generated"
else
    echo -e "\033[31m TestBench FAILED \033[0m"
    failnum=`expr $fail + 1`
fi

if [ -f ./bin/ModelTrain ]
then
    echo " ModelTrain generated"
else
    echo -e "\033[31m ModelTrain FAILED \033[0m"
    failnum=`expr $fail + 1`
fi

if [ -f ./bin/ScoreNormalization ]
then
    echo " ScoreNormalization generated"
else
    echo -e "\033[31m ScoreNormalization FAILED \033[0m"
    failnum=`expr $fail + 1`
fi

if [ "$failnum" -gt 0 ]
then
    echo -e "\033[31m !!!BUILD FAILED \033[0m"
else
    echo -e "\033[33m !!!BUILD SUCCESS \033[0m"
fi
