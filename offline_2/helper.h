#include <algorithm>
#include<stdio.h>

#define n_params 7

using namespace std;

int get_idx(bool arr[], int n, bool elem)
{
	auto itr = find(arr, arr + n, elem);
 
    if (itr != (arr+n))
    {
        return distance(arr, itr);
    }
    else 
    {
        return -1;
    }
}


char* int_to_char(int N)
{
    // Count digits in number N
    int m = N;
    int digit = 0;
    
    while (m)
    {
        digit++;                                // Increment number of digits
        m /= 10;                                // Truncate the last digit from the number     
    }

    char* arr;                                  // Declare char array for result
    char arr1[digit];                           // Declare duplicate char array

    arr = (char*)malloc(digit);                 // Memory allocation of array

    // Separating integer into digits and accommodate it to character array
    int index = 0;
    
    while (N)
    {

        arr1[++index] = N % 10 + '0';           // Separate last digit from the number and add ASCII value of character '0' is 48
        N /= 10;                                // Truncate the last digit from the number
    }

    // Reverse the array for result
    int i;
    for (i = 0; i < index; i++) 
    {
        arr[i] = arr1[index - i];
    }

    arr[i] = '\0';                              // Char array truncate by null

    return (char*)arr;
}


char* get_passenger_name(int id, bool VIP)
{
    if(!VIP)
        return int_to_char(id);                                 // e.g. "1"
    else
    {
        char* pass_id = int_to_char(id);
        char* designation = " (VIP)";
        char* name;

        asprintf(&name, "%s%s", pass_id, designation);          // concatenating

        return name;                                            // e.g. "2 (VIP)"
    }
}


int* get_params(char* filepath)
{
    FILE *fin=fopen(filepath,"r");
    
    if(fin==NULL)
    {
        return new int(-1);
    }

    int* params = new int[n_params];                            // 7 parameters will be provied in the file
    char buff[255];                                             // buffer for reading the input characterwise
    int idx = 0;
    int param;                                              
    
    while(fscanf(fin, "%s", buff)!=EOF)
    {
        sscanf(buff, "%d", &param);                             // from buffer to integer
        params[idx] = param;                                    // putting it into array
        idx++;
    }

    fclose(fin);

    return params;
}