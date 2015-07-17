#include "proton/messenger.h"

typedef struct {
  pn_tracker_t *array;
  size_t count;
  size_t capacity;
} trackerArray;



trackerArray * trackerArray_init(size_t initialSize);

pn_tracker_t trackerArray_get(trackerArray *arr, int pos);

void trackerArray_insert(trackerArray *arr, pn_tracker_t tracker);

int trackerArray_indexOf(trackerArray *arr, pn_tracker_t tracker);

void trackerArray_remove(trackerArray *arr, pn_tracker_t tracker);

void trackerArray_free(trackerArray *arr);