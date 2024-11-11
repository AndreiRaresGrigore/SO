#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  int quantum;
} SchedRRArgs;

void schedRR(FakeOS* os, void* args_){
  SchedRRArgs* args=(SchedRRArgs*)args_;

  for(int i = 0; i < os->cpu_count; i++){
    if(os->cpus[i].running == 0){

      // look for the first process in ready
      // if none, continue with the for cycle 
      if(!os->ready.first) continue;

      FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
      os->cpus[i].running = pcb;

      assert(pcb->events.first);
    ProcessEvent* e = (ProcessEvent*)pcb->events.first;
    assert(e->type==CPU);

    // look at the first event
    // if duration>quantum
    // push front in the list of event a CPU event of duration quantum
    // alter the duration of the old event subtracting quantum
    if (e->duration > args->quantum) {
      ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
      qe->list.prev = qe->list.next = 0;
      qe->type = CPU;
      qe->duration = args->quantum;
      e->duration -= args->quantum;
      List_pushFront(&pcb->events, (ListItem*)qe);
    }

    }
  }

  // look for the first process in ready
  // if none, return
  //if (! os->ready.first)
  //  return;

  //FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  //os->running=pcb;
  
  //assert(pcb->events.first);
  //ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  //assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  /*if (e->duration > args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev = qe->list.next = 0;
    qe->type = CPU;
    qe->duration = args->quantum;
    e->duration -= args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }*/
};

int main(int argc, char** argv) {

  int num_cpu = 1; // NUmero di defalut di cpu
  int id_arg = 1; // Indice per roba da leggere

  // Controlla se l'utente ha specificato il numero di cpu
  if (argc > 1 && strcmp(argv[1], "-cpu") == 0) {
    num_cpu = atoi(argv[2]); // Prendo il numero di cpu inserito
    if (argc > 2 && num_cpu) { //Verifico che sia più lungo di 2 e che ho un intero in quella posizione 
      id_arg = 3; // Cambio indice per roba da leggere a 3 perchè ho -cpu e il numero delle cpu accanto
    } 
    else {
      printf("Errore: valore per -cpu non specificato.\n");
      return -1;
    }
  }

  // Inizializza il sistema operativo con il numero di CPU specificato
  FakeOS_init(&os, num_cpu);

  //FakeOS_init(&os, (int*)argv[1]); //Prendo os e il numero di cpu che inserisco mettendo come seconda stringa del cmd 
  SchedRRArgs srr_args;
  srr_args.quantum=5;
  os.schedule_args=&srr_args;
  os.schedule_fn=schedRR;
  
  
  for (int i = id_arg; i < argc; ++i){ //argc: (Argument Count):conta numero di stinghe scritte in cmd TIPO-> (int)
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]); //argv: (Argument Vector):array di char sritte in cmd TIPO-> (char*)
    printf("loading [%s], pid: %d, events:%d",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);

//SERVE PER VEDERE SE DEVO EFFETTIVAMENTE ENTRARE NEL CICLO DEL WHILE 
  int sto_runnando = 0;
  for(int i = 0; i < os.cpu_count; i++){
    if(os.cpus[i].running != 0){
      sto_runnando = 1;
      break;
    }
  }

  while(sto_runnando
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
    
    //SERVE PER VEDERE SE HO ANCORA DEI RUNNING NELLE DIVERSE CPU 
    sto_runnando = 0;
    for(int i = 0; i < os.cpu_count; i++){
      if(os.cpus[i].running != 0){
        sto_runnando = 1;
        break;
      }
    }

  }
}
