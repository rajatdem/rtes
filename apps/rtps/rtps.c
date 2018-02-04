#include <stdio.h>
#include <stdlib.h>
#include "rtps.h"

void main()
{
    long thread_count = 0;
    int i = 0, j = 0;
    rt_thread_details_t *rt_data;
    rt_thread_details_t temp_data;

    while(1)
    {
        thread_count = count_rt_threads();
        rt_data = (rt_thread_details_t *)malloc(sizeof(rt_thread_details_t)*thread_count);
        if(list_rt_threads(rt_data, thread_count) > 0)
        {
            printf("\n\nTID\tPID\tRT_PR\tName\n");
            
            for(i = 0; i < thread_count; i++)
            {
                for(j = 0; j<thread_count-1; j++)
                {
                    if(rt_data[j].rt_prio < rt_data[j+1].rt_prio)
                    {
                        temp_data = rt_data[j];
                        rt_data[j] = rt_data[j+1];
                        rt_data[j+1] =  temp_data;
                    }
                }
            }
            for(i = 0; i < thread_count; ++i)
            {
                printf("%d\t%d\t%d\t%s\n",\
                        rt_data[i].tid,\
                        rt_data[i].pid,\
                        rt_data[i].rt_prio,\
                        rt_data[i].name);
            }
        }
        free(rt_data);
        sleep(2);
    }
}
