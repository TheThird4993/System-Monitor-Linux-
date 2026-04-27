#ifndef SYSINFOS_H
#define SYSINFOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // p/ sleep
#include <dirent.h> // p/ checar os PIDs

#define BUFSZ 256
#define CPUMODES 10
#define TZ 3       // Time zone, neste caso UTC-3
#define TAMFS 14
#define NAMESTR 20
#define MB 1048576.0

typedef struct SysDetails SysDetails;

SysDetails* sysmonCreate();
void freeAll(SysDetails* sysDet);

void printWebPage(SysDetails*);
void printData(SysDetails*);

int getRTCData(SysDetails*);
int getUptime(SysDetails*);
int getLoadData(SysDetails*);
int getCPUInfo(SysDetails*);
int getCPUUse(SysDetails*);
int getKernelVer(SysDetails*);
int getDistroInfo(SysDetails*);
int getRAMData(SysDetails*);
int getFileSystemData(SysDetails*);
int getStorageDevicesData(SysDetails*);
int getNetworkDeviceData(SysDetails*);
int getDeviceAndGroupData(SysDetails*);
int getProcessData(SysDetails*);

#endif