/*
htop - CPUMeter.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "CPUMeter.h"

#include "CRT.h"
#include "ProcessList.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*{
#include "Meter.h"
}*/

int AllDiskMeter_attributes[] = {
   CPU_NICE, CPU_NORMAL, CPU_KERNEL, CPU_IRQ, CPU_SOFTIRQ, CPU_IOWAIT, CPU_STEAL, CPU_GUEST
};

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static void CPUMeter_init(Meter* this) {
   int cpu = this->param;
   if (this->pl->cpuCount > 1) {
      char caption[10];
      sprintf(caption, "%-3d", ProcessList_cpuId(this->pl, cpu - 1));
      Meter_setCaption(this, caption);
   }
   if (this->param == 0)
      Meter_setCaption(this, "Avg");
}

static void CPUMeter_setValues(Meter* this, char* buffer, int size) {
   ProcessList* pl = this->pl;
   int cpu = this->param;
   if (cpu > this->pl->cpuCount) {
      snprintf(buffer, size, "absent");
      return;
   }
   CPUData* cpuData = &(pl->cpus[cpu]);
   double total = (double) ( cpuData->totalPeriod == 0 ? 1 : cpuData->totalPeriod);
   double percent;
   this->values[0] = cpuData->nicePeriod / total * 100.0;
   this->values[1] = cpuData->userPeriod / total * 100.0;
   if (pl->detailedCPUTime) {
      this->values[2] = cpuData->systemPeriod / total * 100.0;
      this->values[3] = cpuData->irqPeriod / total * 100.0;
      this->values[4] = cpuData->softIrqPeriod / total * 100.0;
      this->values[5] = cpuData->ioWaitPeriod / total * 100.0;
      this->values[6] = cpuData->stealPeriod / total * 100.0;
      this->values[7] = cpuData->guestPeriod / total * 100.0;
      Meter_setItems(this, 8);
      percent = MIN(100.0, MAX(0.0, (this->values[0]+this->values[1]+this->values[2]+
                       this->values[3]+this->values[4])));
   } else {
      this->values[2] = cpuData->systemAllPeriod / total * 100.0;
      this->values[3] = (cpuData->stealPeriod + cpuData->guestPeriod) / total * 100.0;
      Meter_setItems(this, 4);
      percent = MIN(100.0, MAX(0.0, (this->values[0]+this->values[1]+this->values[2]+this->values[3])));
   }
   if (isnan(percent)) percent = 0.0;
   snprintf(buffer, size, "%5.1f%%", percent);
}

static void CPUMeter_display(Object* cast, RichString* out) {
   char buffer[50];
   Meter* this = (Meter*)cast;
   RichString_prune(out);
   if (this->param > this->pl->cpuCount) {
      RichString_append(out, CRT_colors[METER_TEXT], "absent");
      return;
   }
   sprintf(buffer, "%5.1f%% ", this->values[1]);
   RichString_append(out, CRT_colors[METER_TEXT], ":");
   RichString_append(out, CRT_colors[CPU_NORMAL], buffer);
   if (this->pl->detailedCPUTime) {
      sprintf(buffer, "%5.1f%% ", this->values[2]);
      RichString_append(out, CRT_colors[METER_TEXT], "sy:");
      RichString_append(out, CRT_colors[CPU_KERNEL], buffer);
      sprintf(buffer, "%5.1f%% ", this->values[0]);
      RichString_append(out, CRT_colors[METER_TEXT], "ni:");
      RichString_append(out, CRT_colors[CPU_NICE], buffer);
      sprintf(buffer, "%5.1f%% ", this->values[3]);
      RichString_append(out, CRT_colors[METER_TEXT], "hi:");
      RichString_append(out, CRT_colors[CPU_IRQ], buffer);
      sprintf(buffer, "%5.1f%% ", this->values[4]);
      RichString_append(out, CRT_colors[METER_TEXT], "si:");
      RichString_append(out, CRT_colors[CPU_SOFTIRQ], buffer);
      sprintf(buffer, "%5.1f%% ", this->values[5]);
      RichString_append(out, CRT_colors[METER_TEXT], "wa:");
      RichString_append(out, CRT_colors[CPU_IOWAIT], buffer);
      sprintf(buffer, "%5.1f%% ", this->values[6]);
      RichString_append(out, CRT_colors[METER_TEXT], "st:");
      RichString_append(out, CRT_colors[CPU_STEAL], buffer);
      if (this->values[7]) {
         sprintf(buffer, "%5.1f%% ", this->values[7]);
         RichString_append(out, CRT_colors[METER_TEXT], "gu:");
         RichString_append(out, CRT_colors[CPU_GUEST], buffer);
      }
   } else {
      sprintf(buffer, "%5.1f%% ", this->values[2]);
      RichString_append(out, CRT_colors[METER_TEXT], "sys:");
      RichString_append(out, CRT_colors[CPU_KERNEL], buffer);
      sprintf(buffer, "%5.1f%% ", this->values[0]);
      RichString_append(out, CRT_colors[METER_TEXT], "low:");
      RichString_append(out, CRT_colors[CPU_NICE], buffer);
      if (this->values[3]) {
         sprintf(buffer, "%5.1f%% ", this->values[3]);
         RichString_append(out, CRT_colors[METER_TEXT], "vir:");
         RichString_append(out, CRT_colors[CPU_GUEST], buffer);
      }
   }
}

static void AllDISKSMeter_getRange(Meter* this, int* start, int* count) {
   int cpus = this->pl->cpuCount;
   switch(Meter_name(this)[0]) {
      default:
      case 'A': // All
         *start = 0;
         *count = cpus;
         break;
      case 'L': // First Half
         *start = 0;
         *count = (cpus+1) / 2;
         break;
      case 'R': // Second Half
         *start = (cpus+1) / 2;
         *count = cpus / 2;
         break;
   }
}

static void AllDISKSMeter_init(Meter* this) {
   int cpus = this->pl->cpuCount;
   if (!this->drawData)
      this->drawData = calloc(sizeof(Meter*), cpus);
   Meter** meters = (Meter**) this->drawData;
   int start, count;
   AllDISKSMeter_getRange(this, &start, &count);
   for (int i = 0; i < count; i++) {
      if (!meters[i])
         meters[i] = Meter_new(this->pl, start+i+1, (MeterClass*) Class(CPUMeter));
      Meter_init(meters[i]);
   }
   if (this->mode == 0)
      this->mode = BAR_METERMODE;
   int h = Meter_modes[this->mode]->h;
   if (strchr(Meter_name(this), '2'))
      this->h = h * ((count+1) / 2);
   else
      this->h = h * count;
}

static void AllDISKSMeter_done(Meter* this) {
   Meter** meters = (Meter**) this->drawData;
   int start, count;
   AllDISKSMeter_getRange(this, &start, &count);
   for (int i = 0; i < count; i++)
      Meter_delete((Object*)meters[i]);
}

static void AllDISKSMeter_updateMode(Meter* this, int mode) {
   Meter** meters = (Meter**) this->drawData;
   this->mode = mode;
   int h = Meter_modes[mode]->h;
   int start, count;
   AllDISKSMeter_getRange(this, &start, &count);
   for (int i = 0; i < count; i++) {
      Meter_setMode(meters[i], mode);
   }
   if (strchr(Meter_name(this), '2'))
      this->h = h * ((count+1) / 2);
   else
      this->h = h * count;
}

static void SingleColDISKSMeter_draw(Meter* this, int x, int y, int w) {
   Meter** meters = (Meter**) this->drawData;
   int start, count;
   AllDISKSMeter_getRange(this, &start, &count);
   for (int i = 0; i < count; i++) {
      meters[i]->draw(meters[i], x, y, w);
      y += meters[i]->h;
   }
}



MeterClass AllDISKSMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete,
      .display = CPUMeter_display
   },
   .defaultMode = CUSTOM_METERMODE,
   .items = 1,
   .total = 100.0,
   .attributes = AllDiskMeter_attributes, 
   .name = "AllDISKS",
   .uiName = "Disks",
   .caption = "Disks",
   .draw = SingleColDISKSMeter_draw,
   .init = AllDISKSMeter_init,
   .updateMode = AllDISKSMeter_updateMode,
   .done = AllDISKSMeter_done
};
