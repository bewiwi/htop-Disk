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

static void Disk_getListPart(int id,char* part,int* count){
    char line[1024];
    char dev[100] ;
    char mountpoint[100] ;
    int i;

    sprintf( part ,"%s", "Error" );

    FILE* fd = fopen( "/proc/mounts","r");
    if (fd){
        i=0;
        while ( fgets(line,sizeof(line),fd) != NULL )
        {
            sscanf(line,"%s%s",dev,mountpoint);
            if ( dev[0] == '/' )
            {
                if(  i == id){
                   sprintf( part ,"%s", mountpoint );
                }
                i++;
            }
        }
        fclose(fd);
        *count = i;
    }
}



static void DiskMeter_display(Object* cast, RichString* out) {
    char buffer[50];
    Meter* this = (Meter*)cast;

    sprintf(buffer, "%5.1f%% ", this->values[0]);
    RichString_append(out, CRT_colors[HOSTNAME], buffer);
}

static void DiskMeter_setValues(Meter* this, char* buffer, int size) {
    struct statvfs info;
    uint64_t percent;
    int id ;
    char part[100];
    int count;

    id = this->param;
    Disk_getListPart(id,&part,&count);

    statvfs (part,&info); 

    uintmax_t u100 = (info.f_blocks - info.f_bfree)  * 100;
    uintmax_t nonroot_total =info.f_blocks ;
    percent = u100 / nonroot_total + (u100 % nonroot_total != 0);
    this->values[0]= percent;

    snprintf(buffer,size,"%d%%",percent);
}

static void DiskMeter_init(Meter* this) {
    char part[100] ;
    int osef;

    Disk_getListPart(this->param,&part,&osef);
    Meter_setCaption(this,part);
}

static void SingleColDisksMeter_draw(Meter* this, int x, int y, int w) {
   Meter** meters = (Meter**) this->drawData;
   int count;
   char part[100] ;
   Disk_getListPart(0,&part,&count);
   for (int i = 0; i < count ; i++) {
      meters[i]->draw(meters[i], x, y, w);
      y += meters[i]->h;
   }

}

static void AllDisksMeter_init(Meter* this) {
   int count ;  
   char part[100] ;
   Disk_getListPart(0,&part,&count);
   if (!this->drawData)
      this->drawData = calloc(sizeof(Meter*), count-1);
   Meter** meters = (Meter**) this->drawData;
   
   for (int i = 0; i < count  ; i++) {
      if (!meters[i])
         meters[i] = Meter_new(this->pl, i, (MeterClass*) Class(DiskMeter));
      Meter_init(meters[i]);
   }
   if (this->mode == 0)
      this->mode = TEXT_METERMODE;
   int h = Meter_modes[this->mode]->h;
   this->h = h * count ;
}


static void AllDisksMeter_done(Meter* this) {
   Meter** meters = (Meter**) this->drawData;
   int count;
   char part[100] ;
   Disk_getListPart(0,&part,&count);
   for (int i = 0; i < count  ; i++)
      Meter_delete((Object*)meters[i]);
}

static void AllDisksMeter_updateMode(Meter* this, int mode) {
   Meter** meters = (Meter**) this->drawData;
   this->mode = mode;
   int h = Meter_modes[mode]->h;
   int  count;
   char part[100] ;
   Disk_getListPart(0,&part,&count);
   for (int i = 0; i < count; i++) {
      Meter_setMode(meters[i], mode);
   }
   this->h = h * count ;
}

MeterClass DiskMeter_class = {
    .super = {
        .extends = Class(Meter),
        .delete = Meter_delete,
        .display = DiskMeter_display
    },
    .setValues = DiskMeter_setValues, 
    .defaultMode = TEXT_METERMODE,
    .total = 100.0,
    .items = 1,
    .attributes = DiskMeter_attributes,
    .name = "Disk",
    .uiName = "Disk",
    .caption = "Disk : ",
    .init = DiskMeter_init
};


  MeterClass AllDisksMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete,
      .display = DiskMeter_display
   },
   .defaultMode = CUSTOM_METERMODE,
   .items = 1,
   .total = 100.0,
   .attributes = DiskMeter_attributes, 
   .name = "AllDisk",
   .uiName = "AllDisk",
   .caption = "Disks",
   .draw = SingleColDisksMeter_draw,
   .init = AllDisksMeter_init,
   .updateMode = AllDisksMeter_updateMode,
   .done = AllDisksMeter_done
};
