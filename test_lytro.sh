
echo "$(tput setaf 3)-------------------------------------------$(tput sgr0)"
echo "$(tput setaf 3)--       LF2DEPTH TEST                   --$(tput sgr0)"
echo "$(tput setaf 3)-- Author Shan Xu, xushan2011@gamil.com  --$(tput sgr0)"
echo "$(tput setaf 3)-- Copyright (C) 2016-2018               --$(tput sgr0)"
echo "$(tput setaf 3)-------------------------------------------$(tput sgr0)"


#creat a output file
name=$(date '+%y_%m_%d_%s')

#==================LYTRO==================
echo "$(tput setaf 6)--       toymap        --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/toymap.xml >>./out/$name.txt
echo "$(tput setaf 6)--       tulip         --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/tulip.xml  >>./out/$name.txt
echo "$(tput setaf 6)--       guitar        --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/guitar.xml >>./out/$name.txt
echo "$(tput setaf 6)--       office        --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/office.xml >>./out/$name.txt
echo "$(tput setaf 6)--       bus           --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/bus.xml  >>./out/$name.txt
echo "$(tput setaf 6)--       flower2       --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/flower2.xml  >>./out/$name.txt
echo "$(tput setaf 6)--       squirrel      --$(tput setaf 1)[OK]$(tput sgr0)"
./bin/lf2depth ./config/LYTRO/squirrel.xml  >>./out/$name.txt
echo "$(tput setaf 3)-------------Finish------------------$(tput sgr0)"

