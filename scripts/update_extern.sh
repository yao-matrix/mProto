dir_path=`dirname $0`
for file in `ls $dir_path`
do
{
  if test -d $file
  then
    # echo "$file"
    if [ -d "$dir_path/${file}/.svn" ]; then
      cd "$dir_path/$file"
      # echo "$dir_path/$file"
      sudo svn update
      cd -
    elif [ -d "$dir_path/${file}/.git" ]; then
      cd "$dir_path/$file"
      # echo "$dir_path/$file"
      sudo git pull
      cd -
    fi
  fi
} &
done

wait

echo "UPDATE DONE"

exit 0
