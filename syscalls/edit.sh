make edit.exe
make edit
cd ../
cp syscalls/to_fsdir/edit fsdir/
./createfs -i fsdir -o student-distrib/filesys_img
