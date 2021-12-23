//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2021
// Student Name: 이충헌
// Student Number: B811225
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

int numProcess;


struct procEntry {
	char *traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accessesj
	int numIHTNonNULLAcess;	// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
}*procTable;
struct Physical_Memory {
    int pm_id;
    int flag_Data; // 데이터 차있는가
	unsigned vpn;
	int counter;
}*pm;

struct pageTableEntry {
	int frame_n;
	int flag_Data;
	unsigned vpn;
	struct secondary *seconPage;
};

struct secondary {
	int frame_n;
	int flag_Data;
	
};

struct Hashtable {
	int proc_id;
	int frame_n;
	unsigned vpn;
	struct Hashtable *next;
} *HashTable;
//데이터가 처음에 들어오면 page fault가 발생하고 pm에 쓰임
//다음 데이터가 들어오면 다음 frame에 쓰인다.
//그러다가 frame이 가득차면 가장먼저 들어와있던 frame 에서 제거하고 마지막에 넣어준다.
// 들어오는 vpn과 pm의 vpn이 같고 id가 같다면 hit 
void FIFO(int nframe) {
	//printf("%d %d \n",nframe,numProcess);
	unsigned addr,vpn,offset;
	char rw;
	int table_num = 0;
	int frame_pointer = 0;
	 while(!feof(procTable[table_num].tracefp))
	 {
		int frame_num;
		fscanf(procTable[table_num].tracefp,"%x %c\n",&addr,&rw); 
		vpn = addr / (1 << PAGESIZEBITS);
		procTable[table_num].ntraces++;
		for(frame_num=0; frame_num <=nframe; frame_num++){
			if(frame_num == nframe){
				pm[frame_pointer].pm_id = table_num;
				pm[frame_pointer].vpn = vpn;
				procTable[table_num].numPageFault++;
				if(frame_pointer == nframe-1) frame_pointer = 0;
				else frame_pointer++;
				break;
			}

			if(pm[frame_num].flag_Data == 0){  // first insert = fault;
				printf("flag\n");
				pm[frame_num].pm_id = table_num;
				pm[frame_num].flag_Data = 1;
				pm[frame_num].vpn = vpn;
				procTable[table_num].numPageFault++;
				break;
			}
			
			if(pm[frame_num].vpn == vpn  && pm[frame_num].pm_id == table_num){
				procTable[table_num].numPageHit++;
				break;
			}
	 	}
		table_num = (table_num +1) % numProcess;
	 }
}

void OneLevel_LRU(int nframe) {
	unsigned addr,vpn,offset;
	char rw;
	int C = 0;
	int table_num = 0;
	int i;
	for(i=0; i< numProcess; i++) procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << 20));
	 while(!feof(procTable[table_num].tracefp)){
		int frame_num;
		fscanf(procTable[table_num].tracefp,"%x %c\n",&addr,&rw);
		vpn = addr / (1 << PAGESIZEBITS);
		procTable[table_num].ntraces++;
		if(pm[procTable[table_num].firstLevelPageTable[vpn].frame_n].vpn == vpn && pm[procTable[table_num].firstLevelPageTable[vpn].frame_n].pm_id == table_num){
			procTable[table_num].numPageHit++;
			pm[procTable[table_num].firstLevelPageTable[vpn].frame_n].counter = C++;
		}  //hit
		//fault
		else {
			for(frame_num=0; frame_num <=nframe; frame_num++){
				if(frame_num == nframe){
				int min = C;
				int i;
				int min_index = 0;
				for(i=0; i<nframe; i++){
					if(min > pm[i].counter) 
					{
						min = pm[i].counter;
						min_index = i;
					}
				}
				procTable[table_num].firstLevelPageTable[vpn].frame_n = min_index;
				pm[min_index].pm_id = table_num;
				pm[min_index].vpn = vpn;
				pm[min_index].counter = C++;
				procTable[table_num].numPageFault++;
				break;
			}
				if(pm[frame_num].flag_Data == 0){  // first insert = fault;
					printf("flag\n");
					procTable[table_num].firstLevelPageTable[vpn].frame_n = frame_num;
					pm[frame_num].pm_id = table_num;
					pm[frame_num].flag_Data = 1;
					pm[frame_num].vpn = vpn;
					pm[frame_num].counter = C++;
					procTable[table_num].numPageFault++;
					break;
				}
			}
			
		}
		table_num = (table_num +1) % numProcess;
	 }
}
void Two_LevelLRU(int firstLevel, int nframe) {
	unsigned addr,vpn,master,secondary,offset;
	char rw;
	int C = 0;
	int table_num = 0;
	int i,j;
	int secondLevel = VIRTUALADDRBITS - (PAGESIZEBITS + firstLevel);

	for(i=0; i< numProcess; i++) procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << firstLevel));
	for(i=0; i< numProcess; i++){
		for(j=0; j < 1 << firstLevel; j++){
			procTable[i].firstLevelPageTable[j].seconPage = (struct secondary *)malloc(sizeof(struct secondary) * (1 << secondLevel));
		}
	}
	 while(!feof(procTable[table_num].tracefp)){
		int frame_num;
		fscanf(procTable[table_num].tracefp,"%x %c\n",&addr,&rw);
		vpn = addr / (1 << PAGESIZEBITS);
		master = vpn / (1 << secondLevel);
		secondary = vpn % (1 << secondLevel);
		procTable[table_num].ntraces++;
		if(pm[procTable[table_num].firstLevelPageTable[master].seconPage[secondary].frame_n].vpn == vpn && pm[procTable[table_num].firstLevelPageTable[master].seconPage[secondary].frame_n].pm_id == table_num){
			procTable[table_num].numPageHit++;
			pm[procTable[table_num].firstLevelPageTable[master].seconPage[secondary].frame_n].counter = C++;
		}  //hit
		//fault
		else {
			for(frame_num = 0; frame_num <=nframe; frame_num++){
				if(frame_num == nframe){
				int min = C;
				int i;
				int min_index = 0;
				for(i=0; i<nframe; i++){
					if(min > pm[i].counter) 
					{
						min = pm[i].counter;
						min_index = i;
					}
				}
				if(procTable[table_num].firstLevelPageTable[master].flag_Data == 0){
						procTable[table_num].firstLevelPageTable[master].flag_Data =1;
						procTable[table_num].num2ndLevelPageTable++;
				}
				procTable[table_num].firstLevelPageTable[master].seconPage[secondary].frame_n = min_index;
				pm[min_index].pm_id = table_num;
				pm[min_index].vpn = vpn;
				pm[min_index].counter = C++;
				procTable[table_num].numPageFault++;
				break;
			}
				if(pm[frame_num].flag_Data == 0){  // first insert = fault;
					if(procTable[table_num].firstLevelPageTable[master].flag_Data == 0){
						procTable[table_num].firstLevelPageTable[master].flag_Data =1;
						procTable[table_num].num2ndLevelPageTable++;
					}
					procTable[table_num].firstLevelPageTable[master].seconPage[secondary].frame_n = frame_num;
					pm[frame_num].pm_id = table_num;
					pm[frame_num].flag_Data = 1;
					pm[frame_num].vpn = vpn;
					pm[frame_num].counter = C++;
					procTable[table_num].numPageFault++;
					break;
				}
			}
			
		}
		table_num = (table_num +1) % numProcess;
	 }
}
void InvertedHash(int nframe) {
	HashTable = (struct  Hashtable*)malloc(sizeof(struct Hashtable) * nframe);
	unsigned addr,vpn,offset;
	char rw;
	int C = 0;
	int table_num = 0;
	int i;
	int frame_num =0;
	int is_hit = 0;
	 while(!feof(procTable[table_num].tracefp)){
		int node_check = 0;
		fscanf(procTable[table_num].tracefp,"%x %c\n",&addr,&rw);
		vpn = addr / (1 << PAGESIZEBITS);
		procTable[table_num].ntraces++;
		unsigned Hash_index = (vpn + table_num) % nframe;
		if(HashTable[Hash_index].next != NULL){
			procTable[table_num].numIHTNonNULLAcess++;
			struct Hashtable *tmp = malloc(sizeof(struct Hashtable));
			tmp = HashTable[Hash_index].next;
			while(tmp != NULL){
				procTable[table_num].numIHTConflictAccess++;
				if(pm[tmp->frame_n].vpn == vpn && pm[tmp->frame_n].pm_id == table_num){
					procTable[table_num].numPageHit++;
					pm[tmp->frame_n].counter = C++;
					is_hit = 1;
					break;
				}
					tmp = tmp->next;
				
			}
			if(is_hit == 1) 
			{
				table_num = (table_num +1) % numProcess;
				is_hit = 0;
				continue;
			}
		}
		procTable[table_num].numPageFault++;
		if(HashTable[Hash_index].next == NULL){ //처음 삽입 
			procTable[table_num].numIHTNULLAccess++;
			struct Hashtable *node = malloc(sizeof(struct Hashtable));
			if(frame_num < nframe){ // 가득 안찼을때
				pm[frame_num].counter = C++;
				pm[frame_num].pm_id = table_num;
				pm[frame_num].vpn = vpn;
				node->frame_n = frame_num;
				frame_num++;
			}
			else { // 가득염
					int min = C;
					int i;
				    int head = 0;
                    int Hash_index2;
                    struct Hashtable *tmp = malloc(sizeof(struct Hashtable));
                    struct Hashtable *prev = malloc(sizeof(struct Hashtable));
					int min_index = 0;
				for(i=0; i<nframe; i++){
					if(min > pm[i].counter) 
					{
						min = pm[i].counter;
						min_index = i;
					}
				}
				    Hash_index2 = (pm[min_index].pm_id + pm[min_index].vpn) % nframe;
                    tmp = HashTable[Hash_index2].next;
					while(tmp != NULL){
						if(tmp->vpn == pm[min_index].vpn && tmp->proc_id == pm[min_index].pm_id){
							if(head == 0){
								HashTable[Hash_index2].next = tmp->next;
							}
							else{
								prev->next = tmp->next;
							} 
							break;
						}
						else
						{
							prev = tmp;
							tmp = tmp->next;
						}
							head = 1;
						}
				node->frame_n = min_index;
				pm[min_index].pm_id = table_num;
				pm[min_index].vpn = vpn;
				pm[min_index].counter = C++;
			}
			node->proc_id = table_num;
			node->vpn = vpn;
			node->next = NULL;
			HashTable[Hash_index].next = node;
		}
		else{ //처음 삽입이 아닐때 -> 노드의 맨앞으로 추가
			struct Hashtable *node = malloc(sizeof(struct Hashtable));
			if(frame_num < nframe){ // 가득 안찼을때
				pm[frame_num].counter = C++;
				pm[frame_num].pm_id = table_num;
				pm[frame_num].vpn = vpn;
				node->frame_n = frame_num;
				frame_num++;
			}
			else { // 가득염
					int min = C;
					int i;
				    int head = 0;
                    int Hash_index2;
                    struct Hashtable *tmp = malloc(sizeof(struct Hashtable));
                    struct Hashtable *prev = malloc(sizeof(struct Hashtable));
					int min_index = 0;
					for(i=0; i<nframe; i++){
						if(min > pm[i].counter) 
						{
							min = pm[i].counter;
							min_index = i;
						}
					}
				    Hash_index2 = (pm[min_index].pm_id + pm[min_index].vpn) % nframe;
                    tmp = HashTable[Hash_index2].next;
					while(tmp != NULL){
						if(tmp->vpn == pm[min_index].vpn && tmp->proc_id == pm[min_index].pm_id){
							if(head == 0){
								HashTable[Hash_index2].next = tmp->next;
							}
							else{
								prev->next = tmp->next;
							} 
							break;
						}
						else
						{
							prev = tmp;
							tmp = tmp->next;
						}
							head = 1;
						}
				node->frame_n = min_index;
				pm[min_index].pm_id = table_num;
				pm[min_index].vpn = vpn;
				pm[min_index].counter = C++;
			}
			node->proc_id = table_num;
			node->vpn = vpn;
			node->next = HashTable[Hash_index].next;
			HashTable[Hash_index].next = node;
		}
		table_num = (table_num +1) % numProcess;
	 }
}
void oneLevelVMSim() {
    int i;
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}
void twoLevelVMSim() {
	int i;
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void invertedPageVMSim() {
    int i;
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
}

int main(int argc, char *argv[]) {
	int i,c, simType;
    int firstL_bit;
    int phyMemSizeBits;
    int nFrame;  //Frame 갯수
    int optind;

	 numProcess = argc - 4;
	 procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);
     simType = atoi(argv[1]);
     firstL_bit = atoi(argv[2]);
     phyMemSizeBits = atoi(argv[3]);
	 nFrame = 1 << (phyMemSizeBits - PAGESIZEBITS);
	 pm = (struct Physical_Memory *)malloc(sizeof(struct Physical_Memory) * nFrame);
	// initialize procTable for Memory Simulations
	for(i = 0; i < numProcess; i++) {
		optind = 1;
		// opening a tracefile for the process
		printf("process %d opening %s\n",i,argv[i + optind + 3]);
		procTable[i].tracefp = fopen(argv[i + optind + 3],"r");
		procTable[i].traceName = argv[i + optind +3];
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...",argv[i+optind+3]);
			exit(1);
		}
	}
	printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		FIFO(nFrame);
		oneLevelVMSim();
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		OneLevel_LRU(nFrame);
		oneLevelVMSim();
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		Two_LevelLRU(firstL_bit,nFrame);
		twoLevelVMSim();
	}
	
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		InvertedHash(nFrame);
		invertedPageVMSim();
	}

	return(0);
}
