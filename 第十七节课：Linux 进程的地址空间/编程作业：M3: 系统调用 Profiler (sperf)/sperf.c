#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#define str_max 1000
char info[str_max];

typedef struct Sys{
    char name[30];
    double total_time;
}Sys_calls;

Sys_calls *system_calls[100];
int calls_num;

void init(){
    calls_num=0;
    memset((void*)system_calls,0,sizeof(system_calls));
}

//find matched system call's name
Sys_calls *find_calls(char *calls_name){
    for(int i=0;i<calls_num;i++){
        if(strcmp(calls_name,system_calls[i]->name)==0){
            return system_calls[i];
        }
    }
    return NULL;
}


void handle_info(){
    //printf("%s",info);
    //get system calls' name
    char name[30]={0};
    char *name_end=strchr(info,'(');
    if(name_end==NULL) return;
    memcpy(name,info,name_end-info);

    //get system calls' time
    char time_cost_str[100]={0};
    char *time_start,*time_end;
    time_start=strrchr(info,'<');
    time_end=strrchr(info,'>');
    if(time_start==NULL||time_end==NULL) return;
    memcpy(time_cost_str,time_start+1,time_end-time_start-1);
    double time_cost=atof(time_cost_str);

    Sys_calls *Call=find_calls(name);
    if(Call==NULL){
        Call=system_calls[calls_num++]=malloc(sizeof(Sys_calls));
        strcpy(Call->name,name);
    }
    Call->total_time+=time_cost;
    //printf("name:%s time:%lf\n",name,time_cost);
}

//sort(bubble sort) system calls accoring to total time cost
void sort(){
    //printf("sort\n");
    for(int i=0;i<calls_num;i++){
        for(int j=i+1;j<calls_num;j++){
            if(system_calls[i]->total_time<system_calls[j]->total_time){
                Sys_calls *tmp=system_calls[j];
                system_calls[j]=system_calls[i];
                system_calls[i]=tmp;
            }
        }
    }
}

void display(){
    sort();
    double all_calls_time=0,other_calls_time=0;
    for(int i=0;i<calls_num;i++){
        all_calls_time+=system_calls[i]->total_time;
        if(i>=5){
            other_calls_time+=system_calls[i]->total_time;
        }
    }
    for(int i=0;i<5;i++){
        int percent=(system_calls[i]->total_time/all_calls_time)*100+0.5;
        printf("%s: %d%%\n",system_calls[i]->name,percent);
    }
    int percent=(other_calls_time/all_calls_time)*100+0.5;
    printf("others: %d%%\n",percent);
}

long get_time_ms(){//ms
    struct timeval val;
    gettimeofday(&val,NULL);
    return val.tv_sec*1000+val.tv_usec/1000;
}
#define CLEAR_LINE "\033[2K"
#define MOVE_CURSOR_UP "\033[1A"
void clear_screen(){
    for(int i=1;i<=6;i++){
        printf(MOVE_CURSOR_UP);
        printf(CLEAR_LINE);
    }
}

int main(int argc, char *argv[],char *envp[]) {
    printf("Hello, World!\n");
    for (int i = 0; i < argc; i++) {
        assert(argv[i]); // always true
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]); // always true
    init();
    int pipefd[2];//pipe[0] refer to read
    assert(pipe(pipefd)==0);
    long last_time=get_time_ms();
    pid_t pid = fork();
    if(pid == 0){
        //printf("children\n");
        close(pipefd[0]);
        char *newargv[100]={"strace","-T"};


        for(int i=1;i<argc;i++){
            newargv[i+1]=malloc(strlen(argv[i])+1);
            strcpy(newargv[i+1],argv[i]);
            //printf("%s\n",argv[i]);
            //printf("%s\n",newargv[i+1]);
        }
        freopen("/dev/null","w",stdout);//redirect the output
        freopen("/dev/null","w",stderr);
        dup2(pipefd[1],STDERR_FILENO);
        execve("/usr/bin/strace",newargv,envp);

    }
    else{
        //printf("father\n");
        assert(pid!=-1);
        close(pipefd[1]);
        char buf[str_max]={0};
        int output_flag=0;//whether there is output in terminal
        while(read(pipefd[0],buf,200)!=0){
            strcat(info,buf);
            if(info[strlen(info)-1]=='\n'){
                handle_info();
                memset(info,0,sizeof(info));
            }
            memset(buf,0,sizeof(buf));
            long now_time=get_time_ms();
            if(now_time-last_time>=1000){
                if(output_flag==0){
                    output_flag=1;
                }
                else{
                    clear_screen();
                }
                //printf("update\n");
                display();
                last_time=get_time_ms();
            }
        }
        if(output_flag==0){
            output_flag=1;
        }
        else{
            clear_screen();
        }
        display();
    }

    return 0;
}


