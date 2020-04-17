#include "openjvs.h"
#include "buffer.h"
#include "stdio.h"
#include <unistd.h>

int running = 1;

Buffer read_buffer;
/* Setup the IO we are trying to emulate */
JVSCapabilities capabilities;

JVSCapabilities *getCapabilities()
{
  return &capabilities;
}

int main(int argc, char **argv)
{
  JVSStatus retval = OPEN_JVS_ERR_OK;

  signal(SIGINT, handleSignal);

  printf("OpenJVS3\n");

  /* Get the config */
  if (processConfig("docs/global_config", &config) != OPEN_JVS_ERR_OK)
  {
    printf("Map file didn't work panic!\n");
    strcpy(config.devicePath, "/dev/ttyUSB0");
  }

  /* Setup the inputs on the computer */
  if (!initInput())
  {
    printf("Error: Inputs could not be setup properly\n");
    return 1;
  }

  /* Setup the JVS Emulator with the RS485 path and capabilities */
  retval = initJVS(config.devicePath, &capabilities);

  if (OPEN_JVS_ERR_OK != retval)
  {
    printf("initJVS() returned:%d \n", retval);
    return 1;
  }

  /* Try to increase prio of JVS communication thread */
  set_realtime_priority(true);

  /* Process packets forever */
  while (running)
  {
    retval = jvs_do();

#ifdef OFFLINE_MODE
    // Give time for debug prints of task started later
    sleep(1);
    retval = OPEN_JVS_ERR_OFFLINE;
    //return 0;
#endif

    switch (retval)
    {
    /* Status that are normal */
    case OPEN_JVS_ERR_OK:
    case OPEN_JVS_ERR_TIMEOUT:
    case OPEN_JVS_ERR_SYNC_BYTE:
    case OPEN_JVS_NO_RESPONSE:
    case OPEN_JVS_ERR_WAIT_BYTES:
      // todo: Checksum error can occur when arcade device is rebooting therefore these are non critical but for testing I want to see all error that are not normal
      //case OPEN_JVS_ERR_CHECKSUM:
      {
        // if (debugEnabled)
        // {
        //   printf("jvs main loop returned:%d \n", retval);
        // }
      }
      break;

    /* Errors */
    default:
    {
      printf("***** jvs main loop returned:%d ****\n", retval);

      // todo: uncomment later once all non-critical errors defined
      //running = false;
    }
    }
  }

  /* Close the file pointer */
  if (!disconnectJVS())
  {
    printf("Error: Couldn't disconnect from serial\n");
    return 1;
  }

  return 0;
}

void handleSignal(int signal)
{
  if (signal == 2)
  {
    running = false;
    printf("Debug: closing\n");
    //stopThreads();
    exit(EXIT_SUCCESS);
  }
}
