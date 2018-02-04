#include<linux/linkage.h>
#include<linux/sched.h>
#include<linux/syscalls.h>
#include<linux/errno.h>
#include<linux/string.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include<linux/kernel.h>
#include<linux/rtes.h>

char *calculator(char *param1, char *param2, char operation)
{
	int i, carry = 0;
	int result_dec_add = 0, result_dec_sub = 0;
    int decimal_length;
	char copy[10];
	char param1_int[12];
	char param2_int[12];
	char dec_param1[6];
	char dec_param2[6];
	char dec_result[3];
    char* final = (char*) kcalloc(20,sizeof(char), GFP_KERNEL);
	unsigned long int_param1;
	unsigned long int_param2;
	unsigned long result_int;
	unsigned long result_dec;

    strcpy(dec_param1, (strchr(param1, '.')  == NULL ) ? "0" : (strchr(param1, '.') + 1));
	strcpy(dec_param2, (strchr(param2, '.')  == NULL ) ? "0" : (strchr(param2, '.') + 1));
	strcpy(dec_result, "");

	for(i=0;i<strlen(param1);i++){

		if(param1[i] == '.')
			break;
		
		if((param1[i] >= 48) && (param1[i] <= 57))
			copy[i] = param1[i];	
			
		else
			return NULL;
	}
	copy[i] = '\0';
    kstrtol(copy, 10, &int_param1);


	for(i=0;i<strlen(param2);i++){
		if(param2[i] == '.')
			break;
		
		if((param2[i] >= 48) && (param2[i] <= 57))
			copy[i] = param2[i];
		
		else
			return NULL;
	}
	copy[i] = '\0';
    kstrtol(copy,10, &int_param2);

	for(i=0;i<strlen(dec_param1);i++){
		
		if((dec_param1[i] >= 48) && (dec_param1[i] <= 57))
			copy[i] = dec_param1[i];
		
		else
			return NULL;
	}
	copy[i] = '\0';
	strcpy(dec_param1, copy);

	for(i=0;i<strlen(dec_param2);i++){
		
		if((dec_param2[i] >= 48) && (dec_param2[i] <= 57))
			copy[i] = dec_param2[i];
		
		else
			return NULL;
	}
	copy[i] = '\0';
	strcpy(dec_param2, copy);

	if(((int_param1 ^ 0xffff) >65535) || ((int_param2 ^ 0xffff) >65535))
		return NULL;

	if((strlen(dec_param1)>3) ||  (strlen(dec_param2)>3))
		return NULL;

    decimal_length = (strlen(dec_param1)>strlen(dec_param2))?(strlen(dec_param1)):(strlen(dec_param2));

	if((operation != '+') && (operation!= '-' ) && (operation!='*' ) && (operation!='/')){
	    return NULL;
	}

	else{
		switch(operation){
			case '+': {
				for(i = decimal_length - 1; i>=0; i--){
                    result_dec_add = (i <= (strlen(dec_param1)-1) ? (dec_param1[i] - '0') : 0) + 
                    	(i <= (strlen(dec_param2)-1) ? (dec_param2[i] - '0') : 0) + carry;

                    carry = result_dec_add / 10;
                    result_dec_add = result_dec_add % 10;

                    dec_result[i] = result_dec_add + '0';
				}
				dec_result[decimal_length] = '\0';
				result_int = int_param1 + int_param2 + carry;
				sprintf(final, "%d.%s", (int)result_int, dec_result);
				printk("answer: %s", final ); 
			break;
			}

			case '-':{
				for(i = decimal_length - 1; i>=0; i--){
                    result_dec_sub = (i <= (strlen(dec_param1)-1) ? ((dec_param1[i] - '0') - carry) : 0) - 
                    	(i <= (strlen(dec_param2)-1) ? (dec_param2[i] - '0') : 0);

                    if(result_dec_sub < 0){
                    	if(dec_param1[i] == 0)
                    		dec_param1[i] = '0';
	                    result_dec_sub = (i <= (strlen(dec_param1)-1) ? ((dec_param1[i] - '0')+ 10 - carry) : 0) - 
                    		(i <= (strlen(dec_param2)-1) ? ((dec_param2[i] - '0')) : 0);
                    	carry = 1;

                    }
                    else{
                    	carry = 0;
                    }
                
                    dec_result[i] = result_dec_sub + '0';
				}
				dec_result[decimal_length] = '\0';
				result_int = int_param1 - int_param2 - carry;

				sprintf(final, "%d.%s", (int)result_int, dec_result);
			break;
		}
			case '*': {				
                unsigned int decimal_count = 1;
                unsigned long mux, mux1, mux2, decimal;
                char dec_result_sized[10] = {0};
		char tmp_mul[20] ={0};
                char tmp_int[20] = {0};
                char tmp_dec[20] = {0};
               
                
                decimal_length = strlen(dec_param1) + strlen(dec_param2);
                for(i =0; i< decimal_length; i++)
                    decimal_count*= 10;
                sprintf(param1_int, "%ld", int_param1);
                strcat(param1_int, dec_param1);
                sprintf(param2_int, "%ld", int_param2);
                strcat(param2_int, dec_param2);
                
		kstrtol(param1_int, 10, &mux1);
		kstrtol(param2_int, 10, &mux2);
                mux = mux1 * mux2;
                //The mult of the entire number with the . 
                sprintf(tmp_mul, "%ld", mux);
                memcpy(tmp_int, tmp_mul, (strlen(tmp_mul) - (decimal_length)));
                strcat(tmp_int, "\0");
		memcpy(tmp_dec, tmp_mul+strlen(tmp_int), 3);
		strcat(tmp_dec, "\0");
                decimal = mux%decimal_count;
                mux = mux/decimal_count;
                //To round off to 16 bit integer and 10 bit decimal
                mux = (mux & 0xffff);
                sprintf(final, "%ld.%s", mux, tmp_dec);
            break;
        }
        	case '/': {
                unsigned int decimal_count = 1;
                char tmp_div[20];
                char final_dec[20];

                //get the max no. of decimal places from 2 inputs
                decimal_length = (strlen(dec_param1) > strlen(dec_param2))?
                                      (strlen(dec_param1)):(strlen(dec_param2));
                for(i = 0; i< decimal_length; i++)
                    decimal_count*= 10;

                sprintf(param1_int, "%ld", int_param1);
                sprintf(param2_int, "%ld", int_param2);
                strcat(param1_int, dec_param1);
                strcat(param2_int, dec_param2);
                for(i =0 ; i<(decimal_length-strlen(dec_param1)); i++)
                	strcat(param1_int,"0");
                for(i = 0; i<(decimal_length-strlen(dec_param2)); i++)	
                	strcat(param2_int,"0");
                kstrtol(param1_int, 10, &int_param1);
                kstrtol(param2_int, 10,&int_param2);
                if(int_param2 == 0){
                	return NULL;
                }
                //printf("%ld  %ld\n", int_param1, int_param2);
                result_int = int_param1/int_param2;
                result_dec = int_param1%int_param2;
                sprintf(final,"%ld", result_int);

                result_dec = (result_dec*1000)/int_param2;
                sprintf(final_dec, "%ld",result_dec);
                sprintf(tmp_div, ".");
                if(result_dec%1000 >= 100){
                	strcat(tmp_div, final_dec);
                } else if(result_dec%1000 >= 10){
                	strcat(tmp_div, "0");
                	strcat(tmp_div, final_dec);
                } else if(result_dec%1000 >= 1){
                	strcat(tmp_div, "00");
                	strcat(tmp_div, final_dec);
                } else{
                	strcat(tmp_div, "000");
                }
                strcat(final, tmp_div);
            break;	
            }   
		}
 	}
    return final;
}

asmlinkage long sys_calc(char __user *param1, char __user *param2, char operation, char __user *result)
{
    char *param1_ker = (char *)kmalloc(10, GFP_KERNEL);
    char *param2_ker = (char *)kmalloc(10, GFP_KERNEL);
    char *result_ker;
    int ret = 0;
#if 0
    if(copy_from_user(param1_ker, param1, sizeof(param1)))
    {
        printk("Unable to copy from user\n");
        return -EINVAL;
    }
    if(copy_from_user(param1_ker, param1, sizeof(param1)))
    {
        printk("Unable to copy from user\n");
        return -EINVAL;
    }
    result_ker = calculator(param1_ker, param2_ker, operation);
#endif
    result_ker = calculator(param1, param2, operation);

	if(NULL == result_ker)
    {
        return -EINVAL;
    }

    ret = copy_to_user(result, result_ker, strlen(result_ker));
    kfree(param1_ker);
    kfree(param2_ker);
    kfree(result_ker);
    return ret;
}
