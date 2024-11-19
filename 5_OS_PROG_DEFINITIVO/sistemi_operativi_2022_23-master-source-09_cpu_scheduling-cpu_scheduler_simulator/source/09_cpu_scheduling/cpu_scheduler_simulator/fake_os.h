#include "fake_process.h"
#include "linked_list.h"
#pragma once


typedef struct {
  ListItem list;
  int pid;
  ListHead events;
  ////////Entrambi calcolati in FakeOS_simStep dove c'è for
  float burst_stimato; /////////Burst stimato dalla formula, float se no me viene sempre 1
  float burst_corrente;/////////Burst corrente 
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args);


typedef struct {
  float alfa;
} SchedSJFArgs;

typedef struct{
  int id;
  FakePCB* running;
} Cpu; //ricordati che i nomi CPU e IO danno fastidio 


typedef struct FakeOS{
  int cpu_count;
  Cpu* cpus;
  //FakePCB* running;
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn;
  void* schedule_args;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os, int cpu_count);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
