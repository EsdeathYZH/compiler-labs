#!/bin/bash
#output color
SYS=$(uname -s)
if [[ $SYS == "Linux" ]]; then
	GREEN_COLOR='\E[1;32m' 
fi

BIN=tiger-compiler
TESTCASEDIR=./testcases
linescount=0
totalsize=0

make >& /dev/null
#echo $?
if [[ $? != 0 ]]; then
	echo -e "${RED_COLOR}[-_-]$ite: Compile Error${RES}"		
	make clean >& /dev/null
	exit 123
fi	
	for tcase in `ls $TESTCASEDIR/`
	do		
		if [ ${tcase##*.} = "tig" ]; then
			tfileName=${tcase##*/}
			./$BIN $TESTCASEDIR/$tfileName &>/dev/null
            cat $TESTCASEDIR/${tfileName}.s | grep -o "addq $\([0-9]\+\), %rsp" > $TESTCASEDIR/temp.txt
			while read line 
            do
                #declare -i size
                size=`echo $line | grep -Eo '[0-9]+'`
                totalsize=$((totalsize+size))
            done < $TESTCASEDIR/temp.txt
            #declare -i lines
            lines=`sed -n '$=' $TESTCASEDIR/${tfileName}.s`
            linescount=$((linescount+lines))
            rm $TESTCASEDIR/${tfileName}.s 
            rm $TESTCASEDIR/temp.txt
		fi
	done

echo -e "${GREEN_COLOR}${ite}Total lines: ${linescount}"
echo -e "${GREEN_COLOR}${ite}Total stack size: ${totalsize}"