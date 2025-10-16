#include <stdlib.h>
#include <string.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {
    if (time == NULL) {
        return TIME_ARRAY_ERROR;
    }

    if (strlen(time) != 6) {
        return TIME_LEN_ERROR;
    }

   
    char buffer[7];
    strncpy(buffer, time, 6);
    buffer[6] = '\0';

    
    char hour_str[3] = {buffer[0], buffer[1], '\0'};
    char minute_str[3] = {buffer[2], buffer[3], '\0'};
    char second_str[3] = {buffer[4], buffer[5], '\0'};

    
    int hour = atoi(hour_str);
    int minute = atoi(minute_str);
    int second = atoi(second_str);

    
    if (hour < 0 || hour > 23 || minute < 0  ||minute > 59 || second < 0 || second > 59) {
        return TIME_VALUE_ERROR;
    }

    
    return (minute * 60) + second;
}
