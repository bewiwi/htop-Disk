/*
htop - DiskMeter.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "DiskMeter.h"

#include "CRT.h"

#include <unistd.h>

#include <sys/statvfs.h>

#include <stdint.h>

/*{
#include "Meter.h"
}*/

int DiskMeter_attributes[] = {
   HOSTNAME
};

static void DiskMeter_setValues(Meter* this, char* buffer, int size) {
   (void) this;
   
  struct statvfs info;
 
  char line[1024];
  char dev[100] ;
  char mountpoint[100] ;
  uint64_t percent;
  FILE* fd = fopen( "/proc/mounts","r");
  if (fd){
      while ( fgets(line,sizeof(line),fd) != NULL )
      {
          sscanf(line,"%s%s",dev,mountpoint);
          char firstchar = dev[0];
          if ( dev[0] == '/' )
          {
//             printf("ok mount : %s on %s \n",dev,mountpoint);
              statvfs (mountpoint,&info); 
              unsigned int sectorSize = info.f_bsize;
              uint64_t usedBytes;
              usedBytes = (info.f_blocks-info.f_bfree)*info.f_bsize;

              /*snprintf(buffer,
                      size,
                      "Mount: %s\nTotal blocks : %d\nFree blocks: %d\nSize of block %d\nSize in bytes: %d\n",
                      mountpoint,
                      info.f_blocks,
                      info.f_bfree,
                      info.f_bsize,
                      usedBytes);*/
              uintmax_t u100 = (info.f_blocks - info.f_bfree)  * 100;
              uintmax_t nonroot_total =info.f_blocks ;
              percent = u100 / nonroot_total + (u100 % nonroot_total != 0);
              snprintf(buffer,size,"%s  %d%%",mountpoint,percent);


          }
      }
      fclose(fd);
  }

}

MeterClass DiskMeter_class = {
    .super = {
        .extends = Class(Meter),
        .delete = Meter_delete
    },
    .setValues = DiskMeter_setValues, 
    .defaultMode = MULTITEXT_METERMODE,
    .total = 100.0,
    .items = 1,
    .attributes = DiskMeter_attributes,
    .name = "Disk",
    .uiName = "Disk",
    .caption = "Disk : ",
};
