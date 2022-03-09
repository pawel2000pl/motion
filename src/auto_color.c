#include "auto_color.h"
#include <omp.h>

void auto_color(unsigned char* ptr, unsigned long int size)
{
    unsigned long long int distr[256] = {0};    
    unsigned long int partial = size / 97 + 1;
    unsigned long int low, high;
    
    #pragma omp parallel
    #pragma omp for
    for (unsigned long int i=0;i<size;i++)
        distr[ptr[i]]++;

    for (unsigned int i=1;i<255;i++)
        distr[i] += distr[i-1];
    
    unsigned long long int highBorder = distr[254]-partial;
    unsigned long long int lowBorder = distr[0]+partial;
        
    for (high=254; high>0 && distr[high] > highBorder; high--);
    for (low=0; low < high && distr[low] < lowBorder; low++);
        
    if (low < high)
    {
        unsigned char* maxPixel = ptr + size;
        #pragma omp parallel
        #pragma omp for
        for (unsigned char* pixel = ptr;pixel<maxPixel;pixel++)
        {
            long int value = (((long int)(*pixel)-(long int)low) << 8) / (long int)(high-low);
            (*pixel) = (value >= 0)?((value <= 255)?value:255):0;
        }
    }
}
