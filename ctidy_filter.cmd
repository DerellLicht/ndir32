make check > output.txt 2>&1
cat output.txt | gawk -f ..\clang-tidy.filter.awk > output.awk.txt
