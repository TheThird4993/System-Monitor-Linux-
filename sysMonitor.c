#include <stdio.h>
#include <stdlib.h>

#define STR_SIZE 32
#define H 0
#define M 1
#define S 3

typedef struct System System;

struct System{
	char kernel_ver[STR_SIZE];
	char distro[STR_SIZE];
	char OS[STR_SIZE];
	int  uptime[3];
};

int main(int argc, char *argv[]){
	System sysinfo;
	int uptimeS = 0;
	char *temp = "";
	FILE *fp = popen("uname -s", "r"); // Nome do OS
	      fscanf(fp, "%[^\n]", sysinfo.OS);
	      
	pclose(fp);

	      fp = popen("cat /etc/os-release | head -n 1", "r"); // ou uname; Versão da distro
	      
	      temp = (char*)calloc(STR_SIZE, sizeof(char));

	      fgets(temp, STR_SIZE, fp);
	      
	      for(int i = 0, j = 0, w = 0; i < STR_SIZE; i++){
	      	if(temp[i] == '"'){
			w = !w;
			continue;
		}
		if(w) sysinfo.distro[j++] = temp[i];
	      }

	pclose(fp);

	      fp = popen("uname -r", "r"); // Versão do kernel
	      fscanf(fp, "%[^\n]", sysinfo.kernel_ver);

	pclose(fp);

	      fp = popen("awk '{print $1}' /proc/uptime", "r");
	      fgets(temp, STR_SIZE, fp);
		
	      for(int i = 0; temp[i] != '\n'; i++){
	      	sysinfo.uptime[S] = (sysinfo.uptime[S] * 10) + (temp[i] - '0');
	      }

	printf("Kernel Version    = %s\n", sysinfo.kernel_ver);
	printf("Distribution Name = %s\n", sysinfo.distro);
	printf("OS Name		  = %s\n", sysinfo.OS);
	printf("uptime real = %s\n", temp);
	printf("Uptime(secs)      = %d\n", sysinfo.uptime[S]);

	free(temp);
 return 0;
}
