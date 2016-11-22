#! /bin/sh


echo "Turning off all notches" 

./phisector /dev/ttyTUFF 0 16 16 
./phisector /dev/ttyTUFF 1 16 16 
./phisector /dev/ttyTUFF 2 16 16 

sleep 1; 

echo turning on  01BH; ./notch /dev/ttyTUFF  3 11 7; sleep 10; 
echo turning on  01BV; ./notch /dev/ttyTUFF  3 5 7; sleep 10; 
echo turning on  01MH; ./notch /dev/ttyTUFF  2 11 7; sleep 10; 
echo turning on  01MV; ./notch /dev/ttyTUFF  2 5 7; sleep 10; 
echo turning on  01TH; ./notch /dev/ttyTUFF  0 11 7; sleep 10; 
echo turning on  01TV; ./notch /dev/ttyTUFF  0 5 7; sleep 10; 
echo turning on  02BH; ./notch /dev/ttyTUFF  3 18 7; sleep 10; 
echo turning on  02BV; ./notch /dev/ttyTUFF  3 12 7; sleep 10; 
echo turning on  02MH; ./notch /dev/ttyTUFF  1 18 7; sleep 10; 
echo turning on  02MV; ./notch /dev/ttyTUFF  1 12 7; sleep 10; 
echo turning on  02TH; ./notch /dev/ttyTUFF  0 18 7; sleep 10; 
echo turning on  02TV; ./notch /dev/ttyTUFF  0 12 7; sleep 10; 
echo turning on  03BH; ./notch /dev/ttyTUFF  3 19 7; sleep 10; 
echo turning on  03BV; ./notch /dev/ttyTUFF  3 13 7; sleep 10; 
echo turning on  03MH; ./notch /dev/ttyTUFF  2 18 7; sleep 10; 
echo turning on  03MV; ./notch /dev/ttyTUFF  2 12 7; sleep 10; 
echo turning on  03TH; ./notch /dev/ttyTUFF  1 19 7; sleep 10; 
echo turning on  03TV; ./notch /dev/ttyTUFF  1 13 7; sleep 10; 
echo turning on  04BH; ./notch /dev/ttyTUFF  2 19 7; sleep 10; 
echo turning on  04BV; ./notch /dev/ttyTUFF  2 13 7; sleep 10; 
echo turning on  04MH; ./notch /dev/ttyTUFF  1 20 7; sleep 10; 
echo turning on  04MV; ./notch /dev/ttyTUFF  1 14 7; sleep 10; 
echo turning on  04TH; ./notch /dev/ttyTUFF  0 19 7; sleep 10; 
echo turning on  04TV; ./notch /dev/ttyTUFF  0 13 7; sleep 10; 
echo turning on  05BH; ./notch /dev/ttyTUFF  3 20 7; sleep 10; 
echo turning on  05BV; ./notch /dev/ttyTUFF  3 14 7; sleep 10; 
echo turning on  05MH; ./notch /dev/ttyTUFF  2 20 7; sleep 10; 
echo turning on  05MV; ./notch /dev/ttyTUFF  2 14 7; sleep 10; 
echo turning on  05TH; ./notch /dev/ttyTUFF  0 20 7; sleep 10; 
echo turning on  05TV; ./notch /dev/ttyTUFF  0 14 7; sleep 10; 
echo turning on  06BH; ./notch /dev/ttyTUFF  3 21 7; sleep 10; 
echo turning on  06BV; ./notch /dev/ttyTUFF  3 15 7; sleep 10; 
echo turning on  06MH; ./notch /dev/ttyTUFF  2 21 7; sleep 10; 
echo turning on  06MV; ./notch /dev/ttyTUFF  2 15 7; sleep 10; 
echo turning on  06TH; ./notch /dev/ttyTUFF  1 21 7; sleep 10; 
echo turning on  06TV; ./notch /dev/ttyTUFF  1 15 7; sleep 10; 
echo turning on  07BH; ./notch /dev/ttyTUFF  2 22 7; sleep 10; 
echo turning on  07BV; ./notch /dev/ttyTUFF  2 16 7; sleep 10; 
echo turning on  07MH; ./notch /dev/ttyTUFF  1 22 7; sleep 10; 
echo turning on  07MV; ./notch /dev/ttyTUFF  1 16 7; sleep 10; 
echo turning on  07TH; ./notch /dev/ttyTUFF  0 21 7; sleep 10; 
echo turning on  07TV; ./notch /dev/ttyTUFF  0 15 7; sleep 10; 
echo turning on  08BH; ./notch /dev/ttyTUFF  3 22 7; sleep 10; 
echo turning on  08BV; ./notch /dev/ttyTUFF  3 16 7; sleep 10; 
echo turning on  08MH; ./notch /dev/ttyTUFF  1 23 7; sleep 10; 
echo turning on  08MV; ./notch /dev/ttyTUFF  1 17 7; sleep 10; 
echo turning on  08TH; ./notch /dev/ttyTUFF  0 22 7; sleep 10; 
echo turning on  08TV; ./notch /dev/ttyTUFF  0 16 7; sleep 10; 
echo turning on  09BH; ./notch /dev/ttyTUFF  3 23 7; sleep 10; 
echo turning on  09BV; ./notch /dev/ttyTUFF  3 17 7; sleep 10; 
echo turning on  09MH; ./notch /dev/ttyTUFF  2 23 7; sleep 10; 
echo turning on  09MV; ./notch /dev/ttyTUFF  2 17 7; sleep 10; 
echo turning on  09TH; ./notch /dev/ttyTUFF  0 23 7; sleep 10; 
echo turning on  09TV; ./notch /dev/ttyTUFF  0 17 7; sleep 10; 
echo turning on  10BH; ./notch /dev/ttyTUFF  3 6 7; sleep 10; 
echo turning on  10BV; ./notch /dev/ttyTUFF  3 0 7; sleep 10; 
echo turning on  10MH; ./notch /dev/ttyTUFF  1 6 7; sleep 10; 
echo turning on  10MV; ./notch /dev/ttyTUFF  1 0 7; sleep 10; 
echo turning on  10TH; ./notch /dev/ttyTUFF  0 6 7; sleep 10; 
echo turning on  10TV; ./notch /dev/ttyTUFF  0 0 7; sleep 10; 
echo turning on  11BH; ./notch /dev/ttyTUFF  3 7 7; sleep 10; 
echo turning on  11BV; ./notch /dev/ttyTUFF  3 1 7; sleep 10; 
echo turning on  11MH; ./notch /dev/ttyTUFF  2 6 7; sleep 10; 
echo turning on  11MV; ./notch /dev/ttyTUFF  2 0 7; sleep 10; 
echo turning on  11TH; ./notch /dev/ttyTUFF  1 7 7; sleep 10; 
echo turning on  11TV; ./notch /dev/ttyTUFF  1 1 7; sleep 10; 
echo turning on  12BH; ./notch /dev/ttyTUFF  2 7 7; sleep 10; 
echo turning on  12BV; ./notch /dev/ttyTUFF  2 1 7; sleep 10; 
echo turning on  12MH; ./notch /dev/ttyTUFF  1 8 7; sleep 10; 
echo turning on  12MV; ./notch /dev/ttyTUFF  1 2 7; sleep 10; 
echo turning on  12TH; ./notch /dev/ttyTUFF  0 7 7; sleep 10; 
echo turning on  12TV; ./notch /dev/ttyTUFF  0 1 7; sleep 10; 
echo turning on  13BH; ./notch /dev/ttyTUFF  3 8 7; sleep 10; 
echo turning on  13BV; ./notch /dev/ttyTUFF  3 2 7; sleep 10; 
echo turning on  13MH; ./notch /dev/ttyTUFF  1 9 7; sleep 10; 
echo turning on  13MV; ./notch /dev/ttyTUFF  1 3 7; sleep 10; 
echo turning on  13TH; ./notch /dev/ttyTUFF  0 8 7; sleep 10; 
echo turning on  13TV; ./notch /dev/ttyTUFF  0 2 7; sleep 10; 
echo turning on  14BH; ./notch /dev/ttyTUFF  3 9 7; sleep 10; 
echo turning on  14BV; ./notch /dev/ttyTUFF  3 3 7; sleep 10; 
echo turning on  14MH; ./notch /dev/ttyTUFF  2 8 7; sleep 10; 
echo turning on  14MV; ./notch /dev/ttyTUFF  2 2 7; sleep 10; 
echo turning on  14TH; ./notch /dev/ttyTUFF  1 10 7; sleep 10; 
echo turning on  14TV; ./notch /dev/ttyTUFF  1 4 7; sleep 10; 
echo turning on  15BH; ./notch /dev/ttyTUFF  3 10 7; sleep 10; 
echo turning on  15BV; ./notch /dev/ttyTUFF  3 4 7; sleep 10; 
echo turning on  15MH; ./notch /dev/ttyTUFF  2 9 7; sleep 10; 
echo turning on  15MV; ./notch /dev/ttyTUFF  2 3 7; sleep 10; 
echo turning on  15TH; ./notch /dev/ttyTUFF  0 9 7; sleep 10; 
echo turning on  15TV; ./notch /dev/ttyTUFF  0 3 7; sleep 10; 
echo turning on  16BH; ./notch /dev/ttyTUFF  2 10 7; sleep 10; 
echo turning on  16BV; ./notch /dev/ttyTUFF  2 4 7; sleep 10; 
echo turning on  16MH; ./notch /dev/ttyTUFF  1 11 7; sleep 10; 
echo turning on  16MV; ./notch /dev/ttyTUFF  1 5 7; sleep 10; 
echo turning on  16TH; ./notch /dev/ttyTUFF  0 10 7; sleep 10; 
echo turning on  16TV; ./notch /dev/ttyTUFF  0 4 7; sleep 10; 

