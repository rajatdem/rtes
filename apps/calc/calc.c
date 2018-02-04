#include <stdio.h>
#include <stdlib.h>
#include "calc.h"

int main(int argc, char* argv[])
{
    char *result = (char *) malloc(sizeof(char)*50);
    int ret;
    
    if(argc != 4)
    {
        printf("Insufficient data\n");
        return -1;
    }
    ret = calc(argv[1],argv[3],argv[2][0],result);

    if(!ret)
        printf("%s\n", result);
    else
        printf("NaN\n");
    
    free(result);
    return 0;
}
