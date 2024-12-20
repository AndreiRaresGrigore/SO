#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fake_os.h"

void FakeOS_init(FakeOS* os, int cpu_count) {
  os->cpu_count = cpu_count;
  os->cpus = (Cpu*)malloc(sizeof(Cpu) * cpu_count);
  for(int i = 0; i < cpu_count; i++){
    os->cpus[i].id = i;
    os->cpus[i].running = 0;
  }
  //os->running=0;
  List_init(&os->ready);
  List_init(&os->waiting);
  List_init(&os->processes);
  os->timer=0;
  os->schedule_fn=0;
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // sanity check
  assert(p->arrival_time==os->timer && "time mismatch in creation");
  // we check that in the list of PCBs there is no
  // pcb having the same pid
  for(int i = 0; i < os->cpu_count; i++){
    assert( (!os->cpus[i].running || os->cpus[i].running->pid!=p->pid) && "pid taken");
  }
  //assert( (!os->running || os->running->pid!=p->pid) && "pid taken");

  ListItem* aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // all fine, no such pcb exists
  FakePCB* new_pcb=(FakePCB*) malloc(sizeof(FakePCB));
  new_pcb->list.next=new_pcb->list.prev=0;
  new_pcb->pid=p->pid;
  new_pcb->events=p->events;
  new_pcb->burst_stimato = 1; //////// Valore iniziale per la stima del burst

  assert(new_pcb->events.first && "process without events");

  // depending on the type of the first event
  // we put the process either in ready or in waiting
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  switch(e->type){
  case CPU:
    List_pushBack(&os->ready, (ListItem*) new_pcb);
    break;
  case IO:
    List_pushBack(&os->waiting, (ListItem*) new_pcb);
    break;
  default:
    assert(0 && "illegal resource");
    ;
  }
}




void FakeOS_simStep(FakeOS* os){
  
  printf("************** TIME: %08d **************\n", os->timer);

  //scan process waiting to be started
  //and create all processes starting now


  ///////Scansione dei processi in attesa di essere avviati
  ///////Crea i processi il cui tempo di arrivo (arrival_time) corrisponde al tempo corrente (os->timer)
  ListItem* aux=os->processes.first;
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    FakeProcess* new_process=0;
    if (proc->arrival_time==os->timer){
      new_process=proc;
    }
    aux=aux->next;
    if (new_process) {
      printf("\tcreate pid:%d\n", new_process->pid);
      new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)new_process);
      FakeOS_createProcess(os, new_process);
      free(new_process);
    }
  }

  // scan waiting list, and put in ready all items whose event terminates

  ///////Gestione dei processi in attesa (waiting)
  ///////Per ogni processo in os->waiting, decrementa la durata dell'evento IO corrente
  ///////Se l'evento termina, gestisce l'evento successivo o termina il processo
  aux=os->waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    printf("\twaiting pid: %d\n", pcb->pid);
    assert(e->type==IO);
    e->duration--;
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend burst\n");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os->waiting, (ListItem*)pcb);
      if (! pcb->events.first) {
        // kill process
        printf("\t\tend process\n");
        free(pcb);
      } else {
        //handle next event
        e=(ProcessEvent*) pcb->events.first;
        switch (e->type){
        case CPU:
          printf("\t\tmove to ready\n");
          List_pushBack(&os->ready, (ListItem*) pcb);
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          List_pushBack(&os->waiting, (ListItem*) pcb);
          break;
        }
      }
    }
  }

  

  // decrement the duration of running
  // if event over, destroy event
  // and reschedule process
  // if last event, destroy running

  ///////Gestione del processo in esecuzione (os->running)
  ///////Decrementa la durata dell'evento CPU corrente
  ////// Se l'evento termina, gestisce l'evento successivo o termina il processo
  for (int i = 0; i < os->cpu_count; i++){  //Inserito questa riga rispetto a originale

    FakePCB* running=os->cpus[i].running; //cambiato os->running con os->cpus[i].running
    printf("\trunning pid on CPU %d: %d\n", i, running ? running->pid:-1); //Aggiunto i(CPU)
    if (running) {
      ProcessEvent* e = (ProcessEvent*) running->events.first;
      assert(e->type == CPU);
      e->duration --;
      printf("\t\tremaining time on CPU %d: %d\n", i, e->duration); //Aggiunto i(CPU)
      if (e->duration == 0){
        printf("\t\tend burst on CPU %d\n", i); //Aggiunto i(CPU)
        float alfa = 0.5;
        //////////Applico la formula del SJF quando la durata dell'evento raggiunge 0 ed ho trovato la lunghezza del burst attuale 
        running->burst_stimato = (int) (alfa * running->burst_corrente + (1-alfa) * running->burst_stimato);/////Castato a int rispetto a prima
        running->burst_corrente = 0; ///////Reset burst corrente
        List_popFront(&running->events);
        free(e);
        if (! running->events.first) {
          printf("\t\tend process pid %d on CPU %d\n", running->pid, i); //aggiunto testo pid
          free(running); // kill process
        } else {
          e=(ProcessEvent*) running->events.first;
          switch (e->type){
          case CPU:
            printf("\t\tmove to ready pid %d\n", running->pid); //aggiunto testo pid
            List_pushBack(&os->ready, (ListItem*) running);
            break;
          case IO:
            printf("\t\tmove to waiting pid %d\n", running->pid); //aggiunto testo pid 
            List_pushBack(&os->waiting, (ListItem*) running);
            break;
          }
        }
        os->cpus[i].running = 0;//cambiato os->running con os->cpus[i].running
      } else{
        running->burst_corrente ++; ////////Aggiunto per incrementare durata il burst corrente se non è l'ultimo evento
      }
    }

  }
  // call schedule, if defined

  /////Chiamata allo scheduler
  /////Se lo scheduler è definito e non c'è un processo in esecuzione, chiama la funzione di scheduling
  if (os->schedule_fn){ // ho levato && ! os->running alla condizione perhce con cpu multiple posso permettermi delle cpu libere 
    (*os->schedule_fn)(os, os->schedule_args); 
  }

  // if running not defined and ready queue not empty
  // put the first in ready to run

  //Non ho piu bisogno di dare manualmente alla cpu il primo processo pronto perche lo fa gia lo scheduler di assegnare i processi alle cpu libere 
  /*
  if (! os->running && os->ready.first) {
    os->running=(FakePCB*) List_popFront(&os->ready);
  }
  */

  ++os->timer;

}

void FakeOS_destroy(FakeOS* os) {
}
