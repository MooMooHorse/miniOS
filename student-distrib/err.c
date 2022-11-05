#include "err.h"

int32_t 
handle_error(uint32_t status){
    switch (status){
    case ERR_EXCEPTION:
        return 256;
    
    default:
        return status;
    }
}
