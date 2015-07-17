#include "proton/message.h"
#include "proton/messenger.h"
#include "tracker-array.h"

#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <jansson.h>

#define check(messenger)                                                \
  {                                                                     \
    if(pn_messenger_errno(messenger))                                   \
    {                                                                   \
      printf("check\n");                                                \
      die(__FILE__, __LINE__, pn_error_text(pn_messenger_error(messenger)));    \
    }                                                                   \
  }                                                                     \


const int MAX_BATCH_SIZE = 1024;


void die(const char *file, int line, const char *text)
{
  printf("dead\n");
  fprintf(stderr, "%s:%i: %s\n", file, line, text);
  exit(1);
}

void currentTimeStr(char * tmStr) 
{
  time_t now;
  struct tm * timeInfo;
  char buffer[80];

  time(&now);
  timeInfo = localtime(&now);
  strftime(buffer, 80, "%F %T %Z", timeInfo);
  strcpy(tmStr, buffer);
}


char * trackerStatusText(pn_status_t status)
{
  char * text;

  switch(status) {
    case PN_STATUS_UNKNOWN:
      text = (char *) "unknown";
      break;
    case PN_STATUS_PENDING:
      text = (char *) "pending";
      break;
    case PN_STATUS_ACCEPTED:
      text = (char *) "accepted";
      break;
    case PN_STATUS_REJECTED:
      text = (char *) "rejected";
      break;
    case PN_STATUS_RELEASED:
      text = (char *) "released";
      break;
    case PN_STATUS_MODIFIED:
      text = (char *) "modified";
      break;
    case PN_STATUS_ABORTED:
      text = (char *) "aborted";
      break;
    case PN_STATUS_SETTLED:
      text = (char *) "settled";
      break;
  }
  return text;
}


char* generateMessageContent() 
{
  int curFloor;
  double weight;
  json_t * json;
  char *result;
  char timestamp[80];

  char * version = (char *) "0.1.0";
  char * eventType = (char *) "lift-status";
  char * deviceId = (char *) "ELEV-2000-MON-2000";
  currentTimeStr(timestamp);
  char * liftId = (char *) "ELEV-2000";
  char * liftStatus = (char *) "ascending";
  srand(time(NULL));
  curFloor = rand() % 12;
  weight = ((rand() % (10000 - 20)) + 20) / 10;

  json = json_object();
  json_object_set_new(json, "version", json_string(version));
  json_object_set_new(json, "eventType", json_string(eventType));
  json_object_set_new(json, "deviceId", json_string(deviceId));
  json_object_set_new(json, "timestamp", json_string(timestamp));
  json_object_set_new(json, "liftId", json_string(liftId));
  json_object_set_new(json, "liftStatus", json_string(liftStatus));
  json_object_set_new(json, "curFloor", json_integer(curFloor));
  json_object_set_new(json, "weight", json_real(weight));

  /*
  json = json_pack("{s:s, s:s, s:s, s:s, s:s, s:s, s:i, s:r}", "version", version, "eventType", eventType, "deviceId", deviceId, 
  	"timestamp", timestamp, "liftId", liftId, "liftStatus", liftStatus, "curFloor", curFloor, "weight", weight);
  */
  result = json_dumps(json, JSON_PRESERVE_ORDER);
  printf("message content in json: %s\n", result);
  return result;
}


pn_message_t * messenger_buildMessage(char *address)
{
  char *msgtext;
  pn_message_t * message;
  pn_data_t * body;

  msgtext = generateMessageContent();
  message = pn_message();
  pn_message_set_address(message, address);
  pn_message_set_content_type(message, (char *) "application/octect-stream");
  pn_message_set_inferred(message, true);

  body = pn_message_body(message);
  pn_data_put_binary(body, pn_bytes(strlen(msgtext), msgtext));

  return message;
}


pn_messenger_t *messenger_init() 
{
  pn_messenger_t * messenger = pn_messenger(NULL);
  pn_messenger_set_outgoing_window(messenger, MAX_BATCH_SIZE);
  pn_messenger_set_blocking(messenger, false);
  pn_messenger_start(messenger);
  return messenger;
}


pn_tracker_t messenger_sendAsync(pn_messenger_t *messenger, pn_message_t *message)
{
  pn_tracker_t tracker;  

  pn_messenger_put(messenger, message);
  check(messenger);
  printf("message put to messenger\n");
  //get the tracker for the latest message inserted into queue. Tracker is used to track message status.
  tracker = pn_messenger_outgoing_tracker(messenger);

  return tracker;
}


void messenger_clearLabelledTrackers(trackerArray *trackers, trackerArray *trackersToDelete)
{
  pn_tracker_t tracker;
  int i = 0;

  for (i=0; i<trackersToDelete->count; i++) {
    tracker = trackerArray_get(trackersToDelete, i);
    trackerArray_remove(trackers, tracker);
    printf("tracker %i removed\n", (int)tracker);
  }
}


void messenger_updateTrackers(pn_messenger_t *messenger, trackerArray *trackers) 
{
  pn_tracker_t tracker;
  pn_status_t status;
  trackerArray *trackersToDelete;
  int i = 0;

  trackersToDelete = trackerArray_init(10);

  for (i=0; i<trackers->count; i++) {
    tracker = trackerArray_get(trackers, i);
    if (tracker != -1) {
      status = pn_messenger_status(messenger, tracker);
      printf("tracker %i status: %s\n", (int)tracker, trackerStatusText(status));

      if (status != PN_STATUS_PENDING) {
        //settle the tracker when the related message was sent out
        pn_messenger_settle(messenger, tracker, 0);
        trackerArray_insert(trackersToDelete, tracker);
      }
    }
  }

  //remove all the trackers for those messages sent out
  messenger_clearLabelledTrackers(trackers, trackersToDelete);
  trackerArray_free(trackersToDelete);
  printf("trackersToDelete freed\n");
}


void messenger_process(pn_messenger_t *messenger, trackerArray *trackers) 
{
  int running = 1;

  while (running) {
    printf("before messenger work\n");
    //send queued messages indefinitely
    pn_messenger_work(messenger, -1);
    printf("after messenger work\n");
    messenger_updateTrackers(messenger, trackers);

    //tear down connections when all messages have been sent out
    if (trackers->count == 0) {
      printf("all messages sent out. Try to stop messenger\n");
      pn_messenger_stop(messenger);
      running = 0;
    }
  }

  //wait until messenger state changes to stopped
  while (messenger && !pn_messenger_stopped(messenger)) {
    pn_messenger_work(messenger, 0);
  }
  printf("messenger stopped\n");
}


void parseConfigFile(char * filename, char * address)
{
  FILE * input;
  char buf[1024];
  char val[100], domainName[100], namespaceName[100], eventHubName[100], policyName[100], sasKey[100];

  input = fopen(filename, "r");
  if (input == NULL) {
    printf("Failed to open file: %s\n", filename);
    return;
  }

  while (fgets(buf, sizeof(buf), input)) {
    if (buf[0] == '\n' || buf[0] == '#' || buf[0] == '\0') continue;

    memset(val, 0, sizeof val);
    char * prop = strtok(buf, ":");
    if (prop == NULL) continue;
    char * value = strtok(NULL, ":");
    strncpy(val, value, strlen(value)-1);
    
    printf("size of %s: %s %d\n", prop, val, (int)strlen(val));

    if (strcmp(prop, "domainName") == 0) {
      strcpy(domainName, val);
      printf("domainName: %s\n", domainName);
    }
    else if (strcmp(prop, "namespaceName") == 0) {
      strcpy(namespaceName, val);
      printf("namespaceName: %s\n", namespaceName);
    }
    else if (strcmp(prop, "eventHubName") == 0) {
      strcpy(eventHubName, val);
      printf("eventHubName: %s\n", eventHubName);
    }
    else if (strcmp(prop, "policyName") == 0) {
      strcpy(policyName, val);
      printf("policyName: %s\n", policyName);
    }
    else if (strcmp(prop, "sasKey") == 0) {
      strcpy(sasKey, val);
      printf("sasKey: %s\n", sasKey);
    }
  }

  sprintf(address, "amqps://%s:%s@%s.%s/%s", policyName, sasKey, namespaceName, domainName, eventHubName);
  
  printf("address: %s\n", address);
  fclose(input);
}


int main(int argc, char ** argv)
{
  printf("Press CTRL-C to stop sending messages\n");
  
  int i = 0;
  //batch size cannot exceed 60. Or, sending will fail.
  int msgCount = 59;
  pn_messenger_t * messenger;
  pn_message_t * message;
  pn_tracker_t tracker;
  trackerArray * trackers;
  pn_message_t * messages[msgCount];
  char * filename = "./gateway.conf";
  char address[200];

  parseConfigFile(filename, address);
  trackers = trackerArray_init(10);
  messenger = messenger_init();

  for (i = 0; i < msgCount; i++) {
    message = messenger_buildMessage(address);
    messages[i] = message;
    
    tracker = messenger_sendAsync(messenger, message);
    trackerArray_insert(trackers, tracker);
    sleep(1);
  }

  messenger_process(messenger, trackers);

  
  pn_messenger_free(messenger);
  printf("messenger freed\n");

  trackerArray_free(trackers);
  printf("tracker array freed\n");

  for (i = 0; i < msgCount; i++) {
    pn_message_free(messages[i]);
  }
  printf("message array freed\n");

  return 0;
}
