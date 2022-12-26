make buddy_traverse.exe
make buddy_traverse
cd ../
cp syscalls/to_fsdir/buddy_traverse fsdir/
./createfs -i fsdir -o student-distrib/filesys_img
