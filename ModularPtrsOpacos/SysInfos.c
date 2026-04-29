#include "SysInfos.h"

typedef long long unsigned int tu; // time units

typedef unsigned long long sc; // sector counts 

typedef unsigned long long bc; // byte counts

typedef struct RTC_Time{
	char time[9];
	int h, m, s;
	char date[11];
	int day, month, year;
}RTC_Time;

typedef struct Uptime{
	int d, h, m, s;
	int tot_s;
}Uptime;

typedef struct Idle{
	int d, h, m, s;
	int tot_s;
}Idle;

typedef struct CPU{
	int numProcs; // /procs/cpuinfo ; catar o número do campo "siblings"
	float mhzProc;
	char *name;
	char *vendor;
	tu idle_prev, total_prev;
	float uso;
}CPU;

typedef struct Storage{
	char *name;
	sc numReads;
	sc numWrites;
	sc secReadPrev;
	sc secWritePrev;
	float readSpeed;
	float writeSpeed;
}Storage;

typedef struct NetworkDevice{
	char *name;
	bc rxBytesPrev;
	bc txBytesPrev;
	float rxSpeed;
	float txSpeed;
}NetworkDevice;

typedef struct Devices{
	char *name;
	int group;
}Devices;

typedef struct Process{
	int pid;
	char *name;
}Process;

struct SysDetails{
	Idle idle;
	Uptime uptime;
	RTC_Time rtc;
	CPU cpu;
	Storage *str;
	NetworkDevice *netdev;
	Devices *charDev;
	Devices *blockDev;
	Process *procs;
	char *kernelVer; 		  				  // versão do kernel
	char *distro; 			  				  // nome da distribuição
	char **fs;  			  				  // vetor de nomes dos fs suportados
	float load_1m, load_5m, load_15m; 		  // carga do sistema nos seguintes tempos
	long int usedMem, totalMem, availableMem; // memória ram
	float usoRAM;							  // o nome ja diz
	int numFs;  			  				  // quantidade de file systems suportados
	int numStr; 			  				  // quantidade de dispositivos de armazenamento
	int numNd;								  // quantidade de network devices
	int numDevCh;							  // quantidade de dispositivos de caractere
	int numDevBlk;							  // quantidade de dispositivos de bloco
	int numProcs;
};

SysDetails* sysmonCreate(){
	return (SysDetails*)calloc(1, sizeof(SysDetails));
}

int getRTCData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = {0};
	FILE *f = fopen("/proc/driver/rtc", "r");

	if (!f){
		printf("/proc/driver/rtc nao pode ser acessado\n");
		return -1;
	}

	fgets(temp_char, BUFSZ, f);

	sscanf(temp_char, "rtc_time %*[:] %s", sysDet->rtc.time); // faz o trabalho de parsing

	fgets(temp_char, BUFSZ, f);

	sscanf(temp_char, "rtc_date %*[:] %s", sysDet->rtc.date);

	//------------------------ Processamento das infos coletadas -------------------------------

	for (int i = 0, cont = 0; sysDet->rtc.date[i] != '\0'; i++){
		if (sysDet->rtc.date[i] == '-'){
			cont++;
			continue;
		}
		switch (cont){
		case 0:
			sysDet->rtc.year = (sysDet->rtc.year * 10) + (sysDet->rtc.date[i] - '0');
			break;
		case 1:
			sysDet->rtc.month = (sysDet->rtc.month * 10) + (sysDet->rtc.date[i] - '0');
			break;
		case 2:
			sysDet->rtc.day = (sysDet->rtc.day * 10) + (sysDet->rtc.date[i] - '0');
			break;
		}
	}

	sysDet->rtc.h = ((sysDet->rtc.time[0] - '0') * 10) + (sysDet->rtc.time[1] - '0');
	sysDet->rtc.h = ((sysDet->rtc.h - TZ + 24) % 24); // de UTC para UTC -3
	sysDet->rtc.m = ((sysDet->rtc.time[3] - '0') * 10) + (sysDet->rtc.time[4] - '0');
	sysDet->rtc.s = ((sysDet->rtc.time[6] - '0') * 10) + (sysDet->rtc.time[7] - '0');

	fclose(f);
	return 0;
}

int getUptime(SysDetails *sysDet){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/uptime", "r");

	if (!f){
		printf("/proc/uptime nao pode ser acessado\n");
		return -2;
	}

	fgets(temp_char, BUFSZ, f);


	int cnt;
	for (cnt = 0, sysDet->uptime.tot_s = 0; temp_char[cnt] != '.'; cnt++){
		sysDet->uptime.tot_s = (sysDet->uptime.tot_s * 10) + (temp_char[cnt] - '0');
	}

	if ((temp_char[cnt + 2] - '0') > 5) sysDet->uptime.tot_s++;

	for (int i = cnt + 3; temp_char[i] != '.'; i++){
		if (temp_char[i] >= 48 && temp_char[i] <= 57)
			sysDet->idle.tot_s = ((sysDet->idle.tot_s * 10) + (temp_char[i] - '0'));
	}

	//------------------------ Processamento das infos coletadas -------------------------------

	if (sysDet->cpu.numProcs){
		sysDet->idle.tot_s /= sysDet->cpu.numProcs;
	}
	else{
		printf("Numero de processadores = 0, use getCPUInfo() primeiro.\n");
		return -2;
	}
	sysDet->idle.d = (sysDet->idle.tot_s / 86400);
	sysDet->idle.h = (sysDet->idle.tot_s / 3600) % 24;
	sysDet->idle.m = (sysDet->idle.tot_s / 60) % 60;
	sysDet->idle.s = (sysDet->idle.tot_s % 60);

	sysDet->uptime.d = (sysDet->uptime.tot_s / 86400);
	sysDet->uptime.h = (sysDet->uptime.tot_s / 3600) % 24;
	sysDet->uptime.m = (sysDet->uptime.tot_s / 60) % 60;
	sysDet->uptime.s = (sysDet->uptime.tot_s % 60);

	fclose(f);
	return 0;
}

int getLoadData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/loadavg", "r");

	if (!f){
		printf("/proc/loadavg nao pode ser acessado\n");
		return -3;
	}

	fgets(temp_char, BUFSZ, f);

	sscanf(temp_char, "%f %f %f", &sysDet->load_1m, &sysDet->load_5m, &sysDet->load_15m);

	fclose(f);
	return 0;
}

int getCPUInfo(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	float mhzAtual = 0, mhzSoma = 0;
	FILE *f = fopen("/proc/cpuinfo", "r");

	if (!f){
		printf("/proc/cpuinfo nao pode ser acessado\n");
		return -4;
	}

	size_t cpuNameSz, vendorNameSz;
	int nameChk = 0;
	while (fgets(temp_char, BUFSZ, f)){
		if (!strncmp(temp_char, "vendor_id", 9)){  // 1
			for (vendorNameSz = 0; temp_char[vendorNameSz++] != '\0';);
			sysDet->cpu.vendor = (char*)calloc(sizeof(char), vendorNameSz);
			sscanf(temp_char, "vendor_id %*[:] %[^\n]", sysDet->cpu.vendor);
		}
		if (!strncmp(temp_char, "model name", 10) && !nameChk){   // 2
			for (cpuNameSz = 0; temp_char[cpuNameSz++] != '\0';);
			sysDet->cpu.name = (char*)calloc(sizeof(char), cpuNameSz);
			sscanf(temp_char, "model name %*[:] %[^\n]", sysDet->cpu.name);
			nameChk = 1;
		}
		if (!strncmp(temp_char, "siblings", 8) && nameChk == 1){ // 3
			sscanf(temp_char, "siblings %*[:] %s", temp_char);

			for (int i = 0; temp_char[i] != '\0'; i++){
				sysDet->cpu.numProcs = (sysDet->cpu.numProcs * 10) + (temp_char[i] - '0');
			}

			nameChk = 2;
		}
		if (!strncmp(temp_char, "cpu MHz", 7)){ // 4
			sscanf(temp_char, "cpu MHz %*[:] %f", &mhzAtual);
			mhzSoma += mhzAtual;
		}
	}
	sysDet->cpu.mhzProc = mhzSoma / sysDet->cpu.numProcs;

	fclose(f);
	return 0;
}

int getCPUUse(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/stat", "r");

	if (!f){
		printf("/proc/stat nao pode ser acessado\n");
		return -5;
	}

	fgets(temp_char, BUFSZ, f);

	tu vet[CPUMODES] = { 0 };
	tu idle_at = 0, total_at = 0;     // at = atual
	tu diff_idle = 0, diff_total = 0; // valores em "jiffies"

	sscanf(temp_char, "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld", &vet[0], &vet[1], &vet[2], &vet[3], &vet[4], &vet[5], &vet[6], &vet[7], &vet[8], &vet[9]);
	for (int i = 0; i < CPUMODES; i++) total_at += vet[i];

	idle_at = vet[3] + vet[4];
	diff_idle = idle_at - sysDet->cpu.idle_prev;
	diff_total = total_at - sysDet->cpu.total_prev;

	sysDet->cpu.uso = ((float)(diff_total - diff_idle) / diff_total) * 100;
	sysDet->cpu.idle_prev = idle_at;
	sysDet->cpu.total_prev = total_at;

	fclose(f);
	return 0;
}

int getKernelVer(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/version", "r");

	if (!f){
		printf("/proc/version nao pode ser acessado\n");
		return -6;
	}

	size_t kernelVerSz;

	fgets(temp_char, BUFSZ, f);
	for (kernelVerSz = 0; temp_char[kernelVerSz++] != '(';);
	sysDet->kernelVer = (char*)calloc(sizeof(char), kernelVerSz);
	sscanf(temp_char, "%[^(]", sysDet->kernelVer);

	fclose(f);
	return 0;
}

int getDistroInfo(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/etc/os-release", "r");

	if (!f){
		printf("/etc/os-release nao pode ser acessado\n");
		return -7;
	}

	size_t distroNameSz;
	char distroName[BUFSZ] = { 0 };

	fgets(temp_char, BUFSZ, f);
	sscanf(temp_char, "%*[^=]=\"%[^\"]", distroName);           // %*[^=] <- jogue fora tudo até chegar no igual;
	// =\" <- "âncora" para remover o = e a aspa;
	//printf("TESTE DISTRO = %s\n", distroName);	              // %[^\"] <- ler tudo até a aspa, menos ela mesma;

	//for(distroNameSz = 0; temp_char[distroNameSz++] != '\0';);
	//sysDet->distro = (char*)calloc(sizeof(char), distroNameSz);
	//sscanf(temp_char, "%[^(]", sysDet->distro);

	sysDet->distro = strdup(distroName);

	fclose(f);
	return 0;
}

int getRAMData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/meminfo", "r");

	if (!f){
		printf("/proc/meminfo nao pode ser acessado\n");
		return -8;
	}
	// usedMem = totalMem - availableMem
	//           MemTotal - MemAvailable
	int cnt;
	for (cnt = 0; fgets(temp_char, BUFSZ, f) || cnt < 2;){
		char temp[NAMESTR] = "";

		sscanf(temp_char, "%[^:]", temp);
		//printf("TESTE = \"%s\"\n", temp);

		if (!strcmp(temp, "MemTotal")){
			sscanf(temp_char, "%*[^:]: %ld", &sysDet->totalMem);
			cnt++;
		}
		else if (!strcmp(temp, "MemAvailable")){
			sscanf(temp_char, "%*[^:]: %ld", &sysDet->availableMem);
			cnt++;
		}
	}
		if (cnt != 2) printf("\nDados da memoria nao coletados\n\n");

	//------------------------ Processamento das infos coletadas -------------------------------

	sysDet->totalMem /= 1024;     // passando de KB -> MB
	sysDet->availableMem /= 1024; //
	sysDet->usedMem = sysDet->totalMem - sysDet->availableMem;
	sysDet->usoRAM = ((float)sysDet->usedMem / sysDet->totalMem) * 100.0;

	fclose(f);
	return 0;
}

int getFileSystemData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/filesystems", "r");

	if (!f){
		printf("/proc/filesystems nao pode ser acessado\n");
		return -9;
	}

	char col1[TAMFS] = { 0 }, col2[TAMFS] = { 0 };
	int capacidade = 10; // valor inicial

	sysDet->fs = (char**)calloc(capacidade, sizeof(char*));

	while (fgets(temp_char, BUFSZ, f)){
		int lido = sscanf(temp_char, "%s %s", col1, col2);
		char *fs_name = NULL;
		if (lido == 1) fs_name = col1;
		if (lido == 2) fs_name = col2;

		if (fs_name){
			if (sysDet->numFs >= capacidade){
				capacidade *= 2;
				sysDet->fs = (char**)realloc(sysDet->fs, capacidade*sizeof(char*));
			}

			sysDet->fs[sysDet->numFs++] = strdup(fs_name); // strdup -> malloc + strcpy
		}
	}

	if (sysDet->numFs < capacidade){
		sysDet->fs = (char**)realloc(sysDet->fs, sysDet->numFs*sizeof(char*));
	}

	fclose(f);
	return 0;
}

int getStorageDevicesData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/diskstats", "r");

	if (!f){
		printf("/proc/diskstats nao pode ser acessado\n");
		return -10;
	}

	int sCapacity = 2; // valor inicial
	sc secReads = 0, secWritten = 0;
	sc tempReads = 0, tempWrites = 0;

	sysDet->str = (Storage*)calloc(sCapacity, sizeof(Storage));

	while (fgets(temp_char, BUFSZ, f)){
		char sName[NAMESTR] = { 0 };
		sscanf(temp_char, "%*d %*d %s %llu %*d %llu %*d %llu %*d %llu %*d %*d", sName, &tempReads, &secReads, &tempWrites, &secWritten);

		if (strncmp(sName, "loop", 4) && strncmp(sName, "ram", 3) && strncmp(sName, "dm-", 3)){
				if (sysDet->numStr >= sCapacity){
					sCapacity *= 2;
					sysDet->str = (Storage*)realloc(sysDet->str, sCapacity*sizeof(Storage));
				}

				sysDet->str[sysDet->numStr].numReads = tempReads;
				sysDet->str[sysDet->numStr].numWrites = tempWrites;

				sysDet->str[sysDet->numStr].secReadPrev = secReads;
				sysDet->str[sysDet->numStr].secWritePrev = secWritten;

				sysDet->str[sysDet->numStr].name = strdup(sName);
				sysDet->numStr++;


				for (int i = 0; i < sysDet->numStr; i++){
					if (!strcmp(sName, sysDet->str[i].name)){
						/* debug
						printf("TESTE1 STR[%d] RODADA [%d] secReadPrev  = %llu\n", i, round, sysDet->str[i].secReadPrev);
						printf("                           secWritePrev = %llu\n", sysDet->str[i].secWritePrev);
						printf("                           secRead      = %llu\n", secReads);
						printf("                           secWrite     = %llu\n", secWritten);
						*/
						sysDet->str[i].readSpeed = ((float)(secReads - sysDet->str[i].secReadPrev) * 512) / MB;
						sysDet->str[i].writeSpeed = ((float)(secWritten - sysDet->str[i].secWritePrev) * 512) / MB;

						sysDet->str[i].numReads = tempReads;
						sysDet->str[i].numWrites = tempWrites;

						break;
			
				}
			}
		}
	}

	if (sysDet->numStr < sCapacity){
		sysDet->str = (Storage*)realloc(sysDet->str, sysDet->numStr*sizeof(Storage));
	}
			
	// --------------------- TESTE ARMAZENAMENTO ----------------------------------------------
	/*
	for(int i = 0; i < sysDet->numStr; i++){
	printf("TESTE2 STR[%d] = numReads: %llu\n", i, sysDet->str[i].numReads);
	printf("                 numWrites: %llu\n", sysDet->str[i].numWrites);
	printf("                 readSpeed: %.2f\n", sysDet->str[i].readSpeed);
	printf("                 writeSpeed: %.2f\n", sysDet->str[i].writeSpeed);
	}
	return -99;
	*/

	fclose(f);
	return 0;
}

int getNetworkDeviceData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/net/dev", "r");

	if (!f){
		printf("/proc/net/dev nao pode ser acessado\n");
		return -11;
	}

		
	int ndCapacity = 2; // valor inicial // nd = networkdevice
	int tempCnt = 0;    // contador para pular as duas linhas do /net/dev
	bc tempRx = 0, tempTx = 0;
	sysDet->netdev = (NetworkDevice*)calloc(ndCapacity, sizeof(NetworkDevice));

	while (fgets(temp_char, BUFSZ, f)){
		if (tempCnt++ > 1){
			char ndName[NAMESTR] = { 0 };
			sscanf(temp_char, "%[^:]: %llu %*s %*s %*s %*s %*s %*s %*s %llu", ndName, &tempRx, &tempTx);

				if (sysDet->numNd >= ndCapacity){
					ndCapacity *= 2;
					sysDet->netdev = (NetworkDevice*)realloc(sysDet->netdev, ndCapacity*sizeof(NetworkDevice));
				}
				sysDet->netdev[sysDet->numNd].rxBytesPrev = tempRx;
				sysDet->netdev[sysDet->numNd].txBytesPrev = tempTx;

				sysDet->netdev[sysDet->numNd].name = strdup(ndName);
				sysDet->numNd++;
			
				for (int i = 0; i < sysDet->numNd; i++){
					if (!strcmp(ndName, sysDet->netdev[i].name)){
						sysDet->netdev[i].rxSpeed = ((float)(tempRx - sysDet->netdev[i].rxBytesPrev)) / MB;
						sysDet->netdev[i].txSpeed = ((float)(tempTx - sysDet->netdev[i].txBytesPrev)) / MB;
					}
				}
			
		}
	}

	if (sysDet->numNd < ndCapacity){
		sysDet->netdev = (NetworkDevice*)realloc(sysDet->netdev, sysDet->numNd*sizeof(NetworkDevice));
	}
		
	fclose(f);
	return 0;
}

int getDeviceAndGroupData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	FILE *f = fopen("/proc/devices", "r");

	if (!f){
		printf("/proc/devices nao pode ser acessado\n");
		return -12;
	}

	int blkCapacity = 2, chCapacity = 2; // valor inicial 
	int tempGroup = 0;
	char flag = 0; // char por que eh so uma flag
	sysDet->blockDev = (Devices*)calloc(blkCapacity, sizeof(Devices));
	sysDet->charDev = (Devices*)calloc(chCapacity, sizeof(Devices));

	while (fgets(temp_char, BUFSZ, f)){

		if (!strcmp(temp_char, "Character devices:\n")){
			flag = 0;
			continue;
		}
		else if (!strcmp(temp_char, "Block devices:\n")){
			flag = 1;
			continue;
		}
		if (*temp_char == '\n') continue;

		char devName[NAMESTR] = { 0 };
		sscanf(temp_char, "%d %s", &tempGroup, devName);

		if (flag){
			if (sysDet->numDevBlk >= blkCapacity){
				blkCapacity *= 2;
				sysDet->blockDev = (Devices*)realloc(sysDet->blockDev, blkCapacity*sizeof(Devices));
			}
			sysDet->blockDev[sysDet->numDevBlk].name = strdup(devName);
			sysDet->blockDev[sysDet->numDevBlk].group = tempGroup;
			sysDet->numDevBlk++;
		}
		else {
			if (sysDet->numDevCh >= chCapacity){
				chCapacity *= 2;
				sysDet->charDev = (Devices*)realloc(sysDet->charDev, chCapacity*sizeof(Devices));
			}
			sysDet->charDev[sysDet->numDevCh].name = strdup(devName);
			sysDet->charDev[sysDet->numDevCh].group = tempGroup;
			sysDet->numDevCh++;
		}
	}

	if (sysDet->numDevBlk < blkCapacity){
		sysDet->blockDev = (Devices*)realloc(sysDet->blockDev, sysDet->numDevBlk*sizeof(Devices));
	}
	else if (sysDet->numDevCh < chCapacity){
		sysDet->charDev = (Devices*)realloc(sysDet->charDev, sysDet->numDevCh*sizeof(Devices));
	}


	//printf("TESTE %d CHRDEVS:\n", sysDet->numDevCh);
	//for(int i = 0; i < sysDet->numDevCh; i++){
	//	printf("BLKDEV[%d] = %s\n", i, sysDet->charDev[i].name);
	//}
	//printf("TESTE %d BLKDEVS:\n", sysDet->numDevBlk);
	//for(int i = 0; i < sysDet->numDevBlk; i++){
	//	printf("BLKDEV[%d] = %s\n", i, sysDet->blockDev[i].name);
	//}

	fclose(f);
	return 0;
}

int getProcessData(SysDetails *sysDet ){
	char temp_char[BUFSZ] = { 0 };
	struct dirent *ent; // <- arquivo do diretório
	DIR *dir;	    // <- ptr do diretório

	dir = opendir("/proc"); //pid e name

	if (!dir){
		printf("/proc nao pode ser acessado\n");
		return-14;
	}

	int path[BUFSZ] = { 0 };
	int pCapacity = 8; // valor inicial 
	sysDet->procs = (Process*)calloc(pCapacity, sizeof(Process));

	while (ent = readdir(dir)){
		if (ent->d_name[0] >= '0' && ent->d_name[0] <= '9'){
			char pid[NAMESTR] = { 0 };
			char pName[BUFSZ] = { 0 };
			strcpy(pid, ent->d_name);

			if (sysDet->numProcs >= pCapacity){
				pCapacity *= 2;
				sysDet->procs = (Process*)realloc(sysDet->procs, pCapacity*sizeof(Process));
			}

			sprintf(temp_char, "/proc/%s/comm", pid);

			FILE *f = fopen(temp_char, "r");
			if (!f){
				printf("pid invalido\n");
				continue;
			}

			fgets(pName, BUFSZ, f);

			fclose(f);

			sysDet->procs[sysDet->numProcs].name = strdup(pName);
			sscanf(pid, "%d", &sysDet->procs[sysDet->numProcs].pid);
			sysDet->numProcs++;
		}
	}
	if (sysDet->numProcs < pCapacity){
		sysDet->procs = (Process*)realloc(sysDet->procs, sysDet->numProcs*sizeof(Process));
	}
	
	closedir(dir);
	return 0;
}

void printData(SysDetails *sysDet){
	printf("Data      	   = %.2d/%.2d/%d\n", sysDet->rtc.day, sysDet->rtc.month, sysDet->rtc.year);
	printf("RTC time  	   = %.2d:%.2d:%.2d\n", sysDet->rtc.h, sysDet->rtc.m, sysDet->rtc.s);
	printf("Uptime    	   = %.2d:%.2d:%.2d:%.2d\n", sysDet->uptime.d, sysDet->uptime.h, sysDet->uptime.m, sysDet->uptime.s);
	printf("Idle      	   = %.2d:%.2d:%.2d:%.2d\n", sysDet->idle.d, sysDet->idle.h, sysDet->idle.m, sysDet->idle.s);
	printf("CPU       	   = %s\n", sysDet->cpu.name);
	printf("Uso       	   = %.2f%%\n", sysDet->cpu.uso);
	printf("SysLoad   	   = (1m):%.2f (5m):%.2f (15m):%.2f\n", sysDet->load_1m, sysDet->load_5m, sysDet->load_15m);
	printf("Vendor    	   = %s\n", sysDet->cpu.vendor);
	printf("NumProcs  	   = %d cores\n", sysDet->cpu.numProcs);
	printf("VelProcs  	   = %.2f MHz\n", sysDet->cpu.mhzProc);
	printf("KernelVer 	   = %s\n", sysDet->kernelVer);
	printf("Distro    	   = %s\n", sysDet->distro);
	printf("Uso RAM            = %ldMB/%ldMB (%.2f%%)\n", sysDet->usedMem, sysDet->totalMem, sysDet->usoRAM);
	printf("\nNetwork devices    = ");

	for (int i = 0; i < sysDet->numNd; i++){
		if (i > 0) {
			printf("                     ");
		}

		printf("Interface %-15s| rx: %.3f MB/s | tx: %.3f MB/s \n", sysDet->netdev[i].name, sysDet->netdev[i].rxSpeed, sysDet->netdev[i].txSpeed);

	}

	printf("\nStorage devices    = ");

	for (int i = 0; i < sysDet->numStr; i++){
		if (i > 0) {
			printf("                     ");
		}
		if (sysDet->str[i].name[strlen(sysDet->str[i].name) - 1] >= '0' && sysDet->str[i].name[strlen(sysDet->str[i].name) - 1] <= '9'){
			printf("Partição %s \n", sysDet->str[i].name);
		}
		else printf("Disco    %s | Leitura: %.3f MB/s | Escrita: %.3f MB/s \n", sysDet->str[i].name, sysDet->str[i].readSpeed, sysDet->str[i].writeSpeed);

	}

	printf("\nFS suportados      = ");

	for (int i = 0; i < sysDet->numFs; i++){
		if (i > 0 && i % 3 == 0) {
			printf("                     ");
		}

		printf("%-10s ", sysDet->fs[i]);

		if (i % 3 == 2 || i == sysDet->numFs - 1) {
			printf("\n");
		}
	}

	printf("\nDevices:\n");
	printf(" Character devices = ");

	for (int i = 0; i < sysDet->numDevCh; i++){
		if (i > 1 && i % 3 == 0) {
			printf("                    ");
		}

		printf("%3d %-10s ", sysDet->charDev[i].group, sysDet->charDev[i].name);

		if (i % 3 == 2 || i == sysDet->numDevCh - 1) {
			//printf("%d\n", i%3);
			printf("\n ");
		}
	}

	printf("\n Block devices     = ");

	for (int i = 0; i < sysDet->numDevBlk; i++){
		if (i > 1 && i % 3 == 0) {
			printf("                    ");
		}

		printf("%3d %-10s ", sysDet->blockDev[i].group, sysDet->blockDev[i].name);

		if (i % 3 == 2 || i == sysDet->numDevBlk - 1) {
			//printf("%d\n", i%3);
			printf("\n ");
		}
	}
	printf("\nProcesses          = ");
	for (int i = 0; i < sysDet->numProcs; i++){
		sysDet->procs[i].name[strlen(sysDet->procs[i].name) - 1] = '\0'; // remover o \n ja imbutido na palavra
		printf("%5d %-16.16s ", sysDet->procs[i].pid, sysDet->procs[i].name);
		if ((i + 1) % 4 == 0 || i == sysDet->numProcs - 1) {
			printf("\n"); // Quebra a linha
			if (i < sysDet->numProcs - 1) {
				printf("                     ");
			}
		}
	}

	printf("\n");

	return;
}

void freeAll(SysDetails *sysDet){
	free(sysDet->cpu.name);
	free(sysDet->cpu.vendor);
	free(sysDet->kernelVer);
	free(sysDet->distro);
	for (int i = 0; i < sysDet->numFs; i++){
		free(sysDet->fs[i]);
	}
	free(sysDet->fs);
	for (int i = 0; i < sysDet->numStr; i++){
		free(sysDet->str[i].name);
	}
	free(sysDet->str);
	for (int i = 0; i < sysDet->numNd; i++){
		free(sysDet->netdev[i].name);
	}
	free(sysDet->netdev);

	for (int i = 0; i < sysDet->numDevBlk; i++){
		free(sysDet->blockDev[i].name);
	}
	free(sysDet->blockDev);
	for (int i = 0; i < sysDet->numDevCh; i++){
		free(sysDet->charDev[i].name);
	}
	free(sysDet->charDev);
	for (int i = 0; i < sysDet->numProcs; i++){
		free(sysDet->procs[i].name);
	}
	free(sysDet->procs);
	free(sysDet);

	return;
}

// Essa parte é full IA, azar
void printWebPage(SysDetails *sysDet) {
	// Cabeçalho CGI obrigatório
	printf("Content-Type: text/html; charset=UTF-8\n\n");

	printf("<!DOCTYPE html>\n");
	printf("<html lang='pt-BR'>\n<head>\n");
	printf("    <meta charset='UTF-8'>\n");
	printf("    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
	printf("    <title>SysMonitor Dashboard</title>\n");
	printf("    <style>\n");
	printf("        body { font-family: 'Segoe UI', sans-serif; background-color: #1e1e2e; color: #cdd6f4; margin: 0; padding: 20px; }\n");
	printf("        .container { max-width: 1200px; margin: auto; }\n");
	printf("        h1, h2 { color: #89b4fa; border-bottom: 2px solid #45475a; padding-bottom: 10px; }\n");
	printf("        .card { background-color: #313244; padding: 20px; border-radius: 8px; margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }\n");
	printf("        .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(450px, 1fr)); gap: 20px; }\n");
	printf("        .procs-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(260px, 1fr)); gap: 10px; font-family: monospace; background: #181825; padding: 15px; border-radius: 5px; }\n");
	printf("        .proc-item { padding: 5px; border-left: 3px solid #f38ba8; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }\n");
	printf("        table { width: 100%%; border-collapse: collapse; margin-top: 10px; }\n");
	printf("        th, td { padding: 10px; text-align: left; border-bottom: 1px solid #45475a; }\n");
	printf("        th { background-color: #45475a; color: #bac2de; }\n");
	printf("        .highlight { color: #a6e3a1; font-weight: bold; }\n");
	printf("        .fs-list { font-family: monospace; color: #a6adc8; line-height: 1.6; }\n");
	printf("    </style>\n");
	printf("</head>\n<body>\n");
	printf("<div class='container'>\n");
	printf("    <h1>🚀 Painel de Monitoramento do Sistema</h1>\n");

	// --- Seção 1: Informações Gerais e Hardware ---
	printf("    <div class='grid-2'>\n");

	// Card do Sistema
	printf("        <div class='card'>\n");
	printf("            <h2>🖥️ Sistema</h2>\n");
	printf("            <p><b>Distribuição:</b> %s</p>\n", sysDet->distro);
	printf("            <p><b>Kernel:</b> %s</p>\n", sysDet->kernelVer);
	printf("            <p><b>Uptime:</b> %02d Dias, %02d:%02d:%02d</p>\n", sysDet->uptime.d, sysDet->uptime.h, sysDet->uptime.m, sysDet->uptime.s);
	printf("            <p><b>Tempo Idle:</b> %02d Dias, %02d:%02d:%02d</p>\n", sysDet->idle.d, sysDet->idle.h, sysDet->idle.m, sysDet->idle.s);
	printf("            <p><b>Data/Hora:</b> %02d/%02d/%d %02d:%02d:%02d</p>\n", sysDet->rtc.day, sysDet->rtc.month, sysDet->rtc.year, sysDet->rtc.h, sysDet->rtc.m, sysDet->rtc.s);
	printf("            <p><b>Carga (Load):</b> %.2f (1m) | %.2f (5m) | %.2f (15m)</p>\n", sysDet->load_1m, sysDet->load_5m, sysDet->load_15m);
	printf("        </div>\n");

	// Card de Hardware
	printf("        <div class='card'>\n");
	printf("            <h2>⚙️ Processamento & Memória</h2>\n");
	printf("            <p><b>CPU:</b> %s</p>\n", sysDet->cpu.name);
	printf("            <p><b>Fabricante:</b> %s</p>\n", sysDet->cpu.vendor);
	printf("            <p><b>Núcleos:</b> %d cores @ %.2f MHz</p>\n", sysDet->cpu.numProcs, sysDet->cpu.mhzProc);
	printf("            <p><b>Uso de CPU:</b> <span class='highlight'>%.2f%%</span></p>\n", sysDet->cpu.uso);
	printf("            <p><b>RAM:</b> <span class='highlight'>%ld MB</span> / %ld MB (%.2f%%)</p>\n", sysDet->usedMem, sysDet->totalMem, sysDet->usoRAM);
	printf("        </div>\n");
	printf("    </div>\n");

	// --- Seção 2: Redes e Armazenamento ---
	printf("    <div class='grid-2'>\n");

	// Tabela de Rede
	printf("        <div class='card'>\n");
	printf("            <h2>🌐 Redes</h2>\n");
	printf("            <table><tr><th>Interface</th><th>RX (Down)</th><th>TX (Up)</th></tr>\n");
	for (int i = 0; i < sysDet->numNd; i++) {
		printf("                <tr><td>%s</td><td>%.3f MB/s</td><td>%.3f MB/s</td></tr>\n",
			sysDet->netdev[i].name, sysDet->netdev[i].rxSpeed, sysDet->netdev[i].txSpeed);
	}
	printf("            </table>\n");
	printf("        </div>\n");

	// Tabela de Armazenamento
	printf("        <div class='card'>\n");
	printf("            <h2>💾 Armazenamento</h2>\n");
	printf("            <table><tr><th>Device</th><th>Leitura</th><th>Escrita</th></tr>\n");
	for (int i = 0; i < sysDet->numStr; i++) {
		int len = strlen(sysDet->str[i].name);
		if (sysDet->str[i].name[len - 1] >= '0' && sysDet->str[i].name[len - 1] <= '9') {
			printf("                <tr><td>%s <small>(Partição)</small></td><td>-</td><td>-</td></tr>\n", sysDet->str[i].name);
		}
		else {
			printf("                <tr><td><b>%s</b></td><td>%.3f MB/s</td><td>%.3f MB/s</td></tr>\n",
				sysDet->str[i].name, sysDet->str[i].readSpeed, sysDet->str[i].writeSpeed);
		}
	}
	printf("            </table>\n");
	printf("        </div>\n");
	printf("    </div>\n");

	// --- Seção 3: FS e Dispositivos ---
	printf("    <div class='grid-2'>\n");

	printf("        <div class='card'>\n");
	printf("            <h2>📂 FS Suportados</h2>\n");
	printf("            <div class='fs-list'>");
	for (int i = 0; i < sysDet->numFs; i++) {
		printf("%s%s", sysDet->fs[i], (i < sysDet->numFs - 1) ? " | " : "");
	}
	printf("            </div>\n");
	printf("        </div>\n");

	printf("        <div class='card'>\n");
	printf("            <h2>🔌 Dispositivos do Sistema</h2>\n");
	printf("            <p><b>Caractere:</b> %d detectados</p>\n", sysDet->numDevCh);
	printf("            <p><b>Bloco:</b> %d detectados</p>\n", sysDet->numDevBlk);
	printf("        </div>\n");
	printf("    </div>\n");

	// --- Seção 4: Processos ---
	printf("    <div class='card'>\n");
	printf("        <h2>⚙️ Processos (%d)</h2>\n", sysDet->numProcs);
	printf("        <div class='procs-grid'>\n");
	for (int i = 0; i < sysDet->numProcs; i++) {
		printf("            <div class='proc-item'><b>[%05d]</b> %s</div>\n",
			sysDet->procs[i].pid, sysDet->procs[i].name);
	}
	printf("        </div>\n");
	printf("    </div>\n");

	printf("</div>\n</body>\n</html>\n");
}
