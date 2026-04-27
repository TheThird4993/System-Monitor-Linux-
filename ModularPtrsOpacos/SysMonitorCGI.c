#include "SysInfos.h"

//#define CGI   // Comente para ter o terminal como output 

int main(int argc, char *argv[]){
	SysDetails *sysDet = sysmonCreate();

	getRTCData(sysDet);				//-- Extração dos dados da RTC
	getCPUInfo(sysDet);				//-- Extração dos dados da CPU
	getCPUUse(sysDet);				//-- Extração dos dados de uso de CPU
	getUptime(sysDet);				//-- Extração dos dados do uptime

	getLoadData(sysDet);			//-- Extração dos dados de load

	//------------------ Extração dos dados do OS -------------------------
	getKernelVer(sysDet);			//-- Parte do Kernel
	getDistroInfo(sysDet);			//-- Parte da Distro
	//---------------------------------------------------------------------

	getRAMData(sysDet);				//-- Extração de dados da memoria RAM 
	getFileSystemData(sysDet);		//-- Extração de dados de FS 
	getStorageDevicesData(sysDet);	//-- Extração de dados de armazenamento
	getNetworkDeviceData(sysDet);	//-- Extração de dados e dispositivos de rede 
	getDeviceAndGroupData(sysDet);	//-- Extração de devices e grupos 
	getProcessData(sysDet);			//-- Extração dos dados e PIDs 

#ifndef CGI
	printData(sysDet);
#else
	printWebPage(sysDet);
#endif

	freeAll(sysDet);
	
	return 0;
}