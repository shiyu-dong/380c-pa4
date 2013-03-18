make
gcc test.c -o test
gcc my_test2.c -o my_test2
./csc test.c > test.3addr
./dce -opt=pre -backend=3addr <test.3addr> test.pre.3addr
./csc my_test2.c > my_test2.3addr
./dce -opt=pre -backend=3addr <my_test2.3addr> my_test2.pre.3addr
python convert.py <test.pre.3addr > test.pre.c
python convert.py <my_test2.pre.3addr > my_test2.pre.c
gcc test.pre.c -o test-pre
gcc my_test2.pre.c -o my_test2-pre
./test > test.out
./test-pre > test-pre.out
diff test.out test-pre.out
./my_test2 > my_test2.out
./my_test2-pre > my_test2-pre.out
diff my_test2.out my_test2-pre.out
