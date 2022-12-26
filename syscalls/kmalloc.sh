make kmalloc.exe
make kmalloc
cd ../
cp syscalls/to_fsdir/kmalloc fsdir/
./createfs -i fsdir -o student-distrib/filesys_img
