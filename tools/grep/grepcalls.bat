grep -o "PROC.*\| call.*" ..\..\code\eplayer4\release\*.cod >funcs_calls.txt

grep -h -o "call.*" ..\..\code\eplayer4\release\*.cod >calls_all.txt
grep -h -o ";.*" calls_all.txt >calls_all_cleaned.txt
sort calls_all_cleaned.txt >calls_all_sorted.txt
uniq calls_all_sorted.txt >calls_uniqe.txt
diff calls_uniqe_ref.txt calls_uniqe.txt >calls_diff.txt
grep -h -o ">.*\|<.*" calls_diff.txt >calls_diff_cleaned.txt
sort calls_diff_cleaned.txt >calls_diff_sorted.txt
