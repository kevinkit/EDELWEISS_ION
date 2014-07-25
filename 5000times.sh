i=1
while [ $i -le 5000 ]
do
  echo $i
./edel_host_second
  i=`expr $i + 1`
done
