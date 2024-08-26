#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>

/*
pid/stat/1:pid
pid/stat/2:comm->name
pid/stat/4:ppid
*/
#define maxchildren 100
#define maxtask 100

typedef struct Task_info{
  int pid;
  char name[100];
}Task;
Task task[5000];

typedef struct Proc_info{
  int pid;
  int ppid;
  char name[100];
  int num_of_children;
  int num_of_task;
  struct Proc_info *children[maxchildren];
  Task *childtask[maxtask];
}Process;
Process proc[5000];


int num_of_proc,total_task=0;
int now_proc=0;

int filter(const struct dirent *dir){
    for(int i=0;i<strlen(dir->d_name);i++){
        if(dir->d_name[i]<'0'||dir->d_name[i]>'9'){
            return 0;
        }
    }
    return 1;
}


//change () into {} of task name
void parentheses_changed(char *s){
    char temp[66]="{";
    strcat(temp,s);
    temp[strlen(temp)-1]='}';
    strcpy(s,temp);
}

void readproc(){
    struct dirent **namelist;
    num_of_proc=scandir("/proc",&namelist,filter,alphasort);
    if(num_of_proc<0) printf("scandir error\n");

    for(int i=0;i<num_of_proc;i++){
        char pid_path[100]="/proc/";
        char name_path[100]=" ",task_path[100]=" ",status_path[100]=" ";
        strcat(pid_path,namelist[i]->d_name);
        strcpy(name_path,pid_path);
        strcpy(task_path,pid_path);
        strcpy(status_path,pid_path);
        strcat(name_path,"/comm");
        strcat(task_path,"/task");
        strcat(status_path,"/status");
        //printf("%s\n",pid_path);
        FILE *fp1=fopen(name_path,"r");
        proc[i].pid=strtol(namelist[i]->d_name,NULL,10);
        char procname[66];
        memset(procname,0,66);
        fgets(procname,66,fp1);
        procname[strlen(procname)-1]='\0';
        strcpy(proc[i].name,procname);

        FILE *status=fopen(status_path,"r");
        while(1){
            char s[100];
            memset(s,0,100);
            fgets(s,100,status);
            if(strncmp(s,"PPid",4)==0){
                char ppid[10];int index=0;
                memset(ppid,0,10);
                for(int j=0;j<strlen(s);j++){
                    if(s[j]>='0'&&s[j]<='9'){
                        ppid[index]=s[j];
                        index++;
                    }
                }
                proc[i].ppid=strtol(ppid,NULL,10);
                break;
            }
        }
        //printf("%d %s %d\n",proc[i].pid,proc[i].name,proc[i].ppid);

        //read task
        struct dirent **tasklist;
        int x=scandir(task_path,&tasklist,filter,alphasort);
        for(int j=0;j<x;j++){
            int task_pid=strtol(tasklist[j]->d_name,NULL,10);
            if(task_pid==proc[i].pid) continue;
            char task_name[66];
            memset(task_name,0,66);
            strcpy(task_name,task_path);
            strcat(task_name,"/");
            strcat(task_name,tasklist[j]->d_name);
            strcat(task_name,"/comm");
            //printf("%s\n",task_stat);
            FILE *fp2=fopen(task_name,"r");
            char name[66];
            memset(name,0,66);
            fgets(name,66,fp2);
            strcpy(task[total_task].name,name);

            parentheses_changed(task[total_task].name);
            //printf("%s\n",task[total_task].name);
            task[total_task].pid=task_pid;
            proc[i].childtask[proc[i].num_of_task]=&task[total_task];
            proc[i].num_of_task++;
            total_task++;
            fclose(fp2);
        }
        fclose(fp1);
   }


}

//return a proc according to the pid
Process* search_proc(int pid){
    for(int i=0;i<num_of_proc;i++){
        if (proc[i].pid==pid)
            return &proc[i];
    }
    printf("proc%d Search process error!\n",pid);
    assert(0);
    return NULL;
}


void buildtree(){
    for(int i=0;i<num_of_proc;i++){
        if(proc[i].ppid==0) continue;
        //printf("%d %s %d\n",proc[i].pid,proc[i].name,proc[i].ppid);
        Process *father=search_proc(proc[i].ppid);
        father->children[father->num_of_children]=&proc[i];
        father->num_of_children++;
    }
}

void init(){
    for(int i=0;i<num_of_proc;i++){
        proc[i].num_of_children=0;
        proc[i].num_of_task=0;
        memset(proc[i].name,0,sizeof(proc[i].name));
        for(int j=0;j<maxchildren;j++){
            proc[i].children[j]=NULL;
        }
    }
    for(int i=0;i<total_task;i++){
        memset(task[i].name,0,sizeof(task[i].name));
    }
}

void exchange_proc(Process *a,Process *b){
    int Size=sizeof(Process);
    Process *temp=malloc(Size);
    memcpy(temp,a,Size);
    memcpy(a,b,Size);
    memcpy(b,temp,Size);
    free(temp);
}

void exchange_task(Task *a,Task *b){
    int Size=sizeof(Task);
    Process *temp=malloc(Size);
    memcpy(temp,a,Size);
    memcpy(a,b,Size);
    memcpy(b,temp,Size);
    free(temp);
}

void Alphasort(){
    for(int i=0;i<num_of_proc;i++){
        for(int j=0;j<proc[i].num_of_children-1;j++){
            for(int k=j+1;k<proc[i].num_of_children;k++){
                if(strcmp(proc[i].children[j]->name,proc[i].children[k]->name)>0){
                    exchange_proc(proc[i].children[j],proc[i].children[k]);
                }
            }
        }
    }
    for(int i=0;i<num_of_proc;i++){
        for(int j=0;j<proc[i].num_of_task-1;j++){
            for(int k=j+1;k<proc[i].num_of_task;k++){
                if(strcmp(proc[i].childtask[j]->name,proc[i].childtask[k]->name)>0){
                    Task *temp=proc[i].childtask[j];
                    proc[i].childtask[j]=proc[i].childtask[k];
                    proc[i].childtask[k]=temp;
                }
            }
        }
    }
}

void Numbersort(){
    for(int i=0;i<num_of_proc;i++){
        for(int j=0;j<proc[i].num_of_children-1;j++){
            for(int k=j+1;k<proc[i].num_of_children;k++){
                if(proc[i].children[j]->pid>proc[i].children[k]->pid){
                    exchange_proc(proc[i].children[j],proc[i].children[k]);
                }
            }
        }
    }
    for(int i=0;i<num_of_proc;i++){
        for(int j=0;j<proc[i].num_of_task-1;j++){
            for(int k=j+1;k<proc[i].num_of_task;k++){
                if(proc[i].childtask[j]->pid>proc[i].childtask[k]->pid){
                    Task *temp=proc[i].childtask[j];
                    proc[i].childtask[j]=proc[i].childtask[k];
                    proc[i].childtask[k]=temp;
                }
            }
        }
    }
}

void printversion(){
    printf("\npstree 1.0");
}

void add_pid_to_name(){
    for(int i=0;i<num_of_proc;i++){
        char temp[10];
        sprintf(temp,"(%d)",proc[i].pid);
        strcat(proc[i].name,temp);
    }
    for(int i=0;i<total_task;i++){
        char temp[10];
        sprintf(temp,"(%d)",task[i].pid);
        strcat(task[i].name,temp);
    }
    return;
}

void printspace(int n){
    for(int i=0;i<n;i++)
        printf(" ");
    return;
}


void printnode(Process *node,int spacewidth){
    if(node->num_of_children==0&&node->num_of_task==0){
        printf("|─%s",node->name);
    }
    else{
        printf("|%s>",node->name);
        int newspacewidth=spacewidth+strlen(node->name)+2;
        for(int i=0;i<node->num_of_children;i++){
            if(i) printspace(newspacewidth);
            printnode(node->children[i],newspacewidth);
            if(i==node->num_of_children-1&&node->num_of_task==0) continue;
            printf("\n");
        }
        for(int i=0;i<node->num_of_task;i++){
            if(i!=0||node->num_of_children!=0) printspace(newspacewidth);
            printf("|%s",node->childtask[i]->name);
            if(i!=node->num_of_task-1) printf("\n");
        }
    }
    return;
}

int main(int argc, char *argv[]) {
  printf("Hello, World!\n");
  for (int i = 0; i < argc; i++) {
    assert(argv[i]); // always true
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]); // always true
  init();
  readproc();
  buildtree();
  Alphasort();
  for(int i=1;i<argc;i++){
    if(!strcmp(argv[i],"-V")||!strcmp(argv[i],"--version")){
        printversion();
        return 0;
    }
    else if(!strcmp(argv[i],"-n")||!strcmp(argv[i],"--numeric-sort")){
        Numbersort();
    }
    else if(!strcmp(argv[i],"-p")||!strcmp(argv[i],"--show-pids")){
        add_pid_to_name();
    }
    else{
        printf("Incomplete or wrong command option!\n");
        return 0;
    }
  }
  printnode(&proc[0],0);
  printf("\n");
  return 0;
}


