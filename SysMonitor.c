#include <stdio.h>
#include <stdlib.h>

#define STR_SIZE 32

int main(int argc, char *argv[]){
        char sys_info[3][STR_SIZE] = {0, 0, 0};
        char *temp = "";
        FILE *fp = popen("uname -s", "r"); // Nome do OS

	if(!fp){
	  printf("comando \"uname -s\" não executado\n");
	  return 1;
	}
              fscanf(fp, "%[^\n]", sys_info[0]);

        pclose(fp);

              fp = popen("cat /etc/os-release | head -n 1", "r"); // ou uname; Versão da distro
	
	if(!fp){
	  printf("comando \"cat /etc/os-release\" não executado\n");
	  return 1;
	}

              temp = (char*)calloc(STR_SIZE, sizeof(char));

              fgets(temp, STR_SIZE, fp);

              for(int i = 0, j = 0, w = 0; i < STR_SIZE; i++){
                if(temp[i] == '"'){
                        w = !w;
                        continue;
                }
                if(w) sys_info[1][j++] = temp[i];
              }

              free(temp);

        pclose(fp);

              fp = popen("uname -r", "r"); // Versão do kernel
	
	if(!fp){
	  printf("comando \"uname -r\" não executado\n");
	  return 1;
	}
              fscanf(fp, "%[^\n]", sys_info[2]);

        pclose(fp);

        for(int i = 0; i < 3; i++){
              printf("data[%d] = %s\n", i, sys_info[i]);
        }

 return 0;
}
