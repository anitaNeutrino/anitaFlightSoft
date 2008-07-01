#! /bin/bash
dev=/dev/
name=bigint
have=0
echo "Drives 15,17,18 and 20 are not connected"
for (( i=1; i<10; i++))
do
  have=0
  for drive in sdg1 sdh1 sdi1 sdj1 sdk1 sdl1 sdm1 sdn1 sdo1 sdp1 sdq1 sdr1 sds1 sdt1 sdu1 sdv1 sdw1 sdx1 sdy1 sdz1 sdaa1 sdab1 sdac1 sdad1 sdae1 sdaf1 sdag1 sdah1 sdai1 sdaj1 sdak1
    do
    label=`e2label "$dev$drive"`
    #echo $label
    dname="$name""0""$i"
    #echo $dname

    if [ "$label" = "$dname" ]
	then
	have=1
	break
    fi
  done
  if [ $have -eq 0 ]
      then
      echo "Missing drive $i"
  fi
done
for ((j=10; j<36; j++))
  do
  have=0
    for drive in sdg1 sdh1 sdi1 sdj1 sdk1 sdl1 sdm1 sdn1 sdo1 sdp1 sdq1 sdr1 sds1 sdt1 sdu1 sdv1 sdw1 sdx1 sdy1 sdz1 sdaa1 sdab1 sdac1 sdad1 sdae1 sdaf1 sdag1 sdah1 sdai1 sdaj1 sdak1
    do
      label=`e2label "$dev$drive"`
      #echo $label
      dname="$name$j"
      #echo $dname
      
      if [ "$label" = "$dname" ]
	  then
	  have=1
	  break
    fi
    done
    if [ $have -eq 0 ]
	then
	echo "Missing drive $j"
    fi
done 
exit 1