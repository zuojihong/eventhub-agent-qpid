#include "tracker-array.h"
#include <stdlib.h>

trackerArray * trackerArray_init(size_t initialSize) 
{
  trackerArray *arr;
  arr = (trackerArray *)malloc(sizeof(trackerArray));
  arr->array = (pn_tracker_t *)malloc(initialSize * sizeof(pn_tracker_t));
  arr->count = 0;
  arr->capacity = initialSize;
  return arr;
}

pn_tracker_t trackerArray_get(trackerArray *arr, int pos)
{
  pn_tracker_t tracker = -1;
  if (pos < arr->count) {
    tracker = arr->array[pos];
  }
  return tracker;
}

void trackerArray_insert(trackerArray *arr, pn_tracker_t tracker)
{
  if (arr->count == arr->capacity) {
    arr->capacity *= 2;
    arr->array = (pn_tracker_t *)realloc(arr->array, arr->capacity * sizeof(pn_tracker_t));
  }
  arr->array[arr->count++] = tracker;
}


int trackerArray_indexOf(trackerArray *arr, pn_tracker_t tracker) 
{
  int i = 0, pos = -1;

  for (i=0; i<arr->count; i++) {
    if (arr->array[i] == tracker) {
      pos = i;
      break;
    }
  }
  return pos; 
}


void trackerArray_remove(trackerArray *arr, pn_tracker_t tracker)
{
  int i = 0, pos = 0;

  pos = trackerArray_indexOf(arr, tracker);
  if (pos >= 0) {
    for (i = pos; i < arr->count - 1; i++) {
      arr->array[i] = arr->array[i+1];
    } 
  }
  arr->count--;
}

void trackerArray_free(trackerArray *arr) 
{
  free(arr->array);
  arr->array = NULL;
  arr->count = arr->capacity = 0;
  free(arr);
}