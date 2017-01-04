#!/bin/bash

function update_repo()
{
    # echo $1
    # echo $2

    for file in `ls $1`
    do
    {
	# echo "$1/$file 1"
	if test -d "$1/$file"
	then
	    # echo "$1/$file 2"
	    if [ -d "$1/${file}/.svn" ]; then
		cd "$1/$file"
		# echo "$1/$file"
		sudo svn update
		cd -
	    elif [ -d "$1/${file}/.git" ]; then
		cd "$1/$file"
		# echo "$1/$file"
		# sudo git reset --hard HEAD
		sudo git pull
		cd -
	    else
	        # echo "$1/$file"
	        i=$(($2+1))
	        if test $i -lt 2; then
		    update_repo "$1/$file" $i
		fi
	    fi
	fi
    } &
    done
    wait
}


dir_path=`dirname $0`
update_repo ${dir_path} 0
echo "UPDATE DONE"
exit 0
