#include <stdlib.h>
#include <stdbool.h>

void swap(char *a, char *b)                                                                                                                                                                       
  {
       if(!a || !b)
           return;

       char temp = *(a);
       *(a) = *(b);
       *(b) = temp;
   }

void reverse(char *str, int length) 
{ 
	int start = 0; 
	int end = length -1; 
	while (start < end) 
	{ 
		swap((str+start), (str+end)); 
		start++; 
		end--; 
	} 
} 

char* itoa(int num, char* str, int base) 
{ 
	int i = 0; 
	bool isNegative = false; 

	if (num == 0) 
	{ 
		str[i++] = '0'; 
		str[i] = '\0'; 
		return str; 
	}

	if (base == 10) 
	{ 
		if (num < 0) {
			isNegative = true;
			num = -num; 
		}
		while (num != 0) 
		{ 
			int rem = num % base; 
			str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
			num = num/base; 
		}
	} else {
		unsigned int truenum = (unsigned int)num;
		while (truenum != 0) 
		{ 
			unsigned int rem = truenum % base; 
			str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
			truenum = truenum/base; 
		}
	}

	if (isNegative == true) 
		str[i++] = '-'; 

	str[i] = '\0';
	reverse(str, i); 
	return str; 
} 