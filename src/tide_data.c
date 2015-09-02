#include <pebble.h>
#include "tide_data.h"

void store_tide_data(TideData *src) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Storing tide data.");
  persist_write_string(NAME, src->name);
  persist_write_string(UNIT, src->unit);
  persist_write_int(N_EVENTS, src->n_events);
  persist_write_data(TIMES, src->times.bytes, sizeof(IntByteArray));
  persist_write_data(HEIGHTS, src->heights.bytes, sizeof(IntByteArray));
  persist_write_data(EVENTS, src->events, MAX_TIDE_EVENTS);
}

bool load_tide_data(TideData *dst) {
	if(persist_exists(NAME)){ //assume that if the name exists then all data is there.
	    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading data from storage");
	    persist_read_string(NAME, dst->name, MAX_NAME_LENGTH);
	    persist_read_string(UNIT, dst->unit, 3);
	    dst->n_events = persist_read_int(N_EVENTS);
	    persist_read_data(TIMES, dst->times.bytes, sizeof(IntByteArray));
	    persist_read_data(HEIGHTS, dst->heights.bytes, sizeof(IntByteArray));
	    persist_read_data(EVENTS, dst->events, MAX_TIDE_EVENTS);
	}

	return persist_exists(NAME);
}

//it is less efficient to find the min/max in two passes but it makes the code more readable
int find_min(int *array, int n_elements){
	int min = array[0];
	for(int i=1; i < n_elements; i++){
	        if(array[i] < min) {
	          min = array[i];
	        }
	    }
	return min;
}

int find_max(int *array, int n_elements){
	int max = array[0];
	for(int i=1; i < n_elements; i++){
	        if(array[i] > max) {
	          max = array[i];
	        }
	    }
	return max;
}


