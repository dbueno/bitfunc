# Should grab field 18, too, but that leaves a ) on the end
# that needs to be removed

for file in *stats; 
#  do cut -d',' -f8,14,15,16,17 $file | cut -d ')' -f1 | uniq > $file.preprocd;
  do cut -d',' -f8,14,15,16,17,18 $file | cut -d ')' -f1 | sort -g -t, -u -k1,1  > $file.preprocd;
done;
