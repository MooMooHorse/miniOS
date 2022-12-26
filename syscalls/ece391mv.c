#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main(){

    uint8_t buf[1024];
    uint8_t arg1[1024];
    uint8_t arg2[1024];
    int32_t i=0,j=0;

    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }

    while(buf[i]!=' '&&i<1024){
        if(buf[i]=='\0'){
            ece391_fdputs(1,(uint8_t*)"too few arguments\n");
            return 3;
        }
        arg1[i]=buf[i];
        i++;
    }
    arg1[i]='\0';
    while(buf[i]==' ') i++;
    while(buf[i]!='\0'){
        arg2[j++]=buf[i++];
    }
    arg2[j]='\0';

    if(-1==ece391_file_rename(arg1,arg2)){
        ece391_fdputs(1,(uint8_t*)"file rename failed\n");
        return 3;
    }


    return 0;
}

