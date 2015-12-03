#include <stdio.h>
#include <math.h>
#include <vector>
#include <unordered_map>

#define LOAD 0
#define STORE 1

//we'll assume a constant miss penalty of 35
#define MISS_PENALTY 35

// You can change the parameters here.  Your program should work for any
//     reasonable values of CACHE_SIZE, BLOCK_SIZE, or ASSOCIATIVITY.

#define ADDR_LEN 64
#define CACHE_SIZE 1024*256
#define BLOCK_SIZE 64
#define ASSOCIATIVITY 4

using namespace std;

// a good place to declare your storage for tags, etc.  Obviously,
//   you don't need to actually store the data.


struct ListNode {
  unsigned long key;
  bool val;
  ListNode* prev;
  ListNode* next;
  ListNode(unsigned long tag, bool in_cache): key(tag), val(in_cache), prev(nullptr), next(nullptr) {}
};

int offset_bits = log2(BLOCK_SIZE);
int idx_num = CACHE_SIZE / (BLOCK_SIZE * ASSOCIATIVITY);
int idx_bits = log2(idx_num);
int tag_bits = ADDR_LEN - offset_bits - idx_bits;

vector<ListNode*> head_vec(idx_num, nullptr);
vector<ListNode*> tail_vec(idx_num, nullptr);
unordered_map<unsigned long, ListNode*> tmp;
vector<unordered_map<unsigned long, ListNode*> > table_vec(idx_num, tmp);

long hits=0, misses=0, read_hits=0, read_misses=0;


unsigned long getIdx(unsigned long addr) {
  return (addr << tag_bits) >> (ADDR_LEN - idx_bits);
}

unsigned long getTag(unsigned long addr) {
  return addr >> (ADDR_LEN - tag_bits);
}


void insertToTail(unsigned long idx, ListNode* node) {
  if (!tail_vec[idx]) {
    head_vec[idx] = node;
  } else {
    (tail_vec[idx])->next = node;
    node->prev = tail_vec[idx];
  }
  tail_vec[idx] = node;
  node->next = nullptr;
  table_vec[idx][node->key] = node;
}

void removeHead(unsigned long idx) {
  ListNode* node = head_vec[idx];
  if (head_vec[idx] == tail_vec[idx]) {
    head_vec[idx] == nullptr;
    tail_vec[idx] = nullptr;
  } else {
    ListNode* tmp = node->next;
    node->next = nullptr;
    tmp->prev = nullptr;
    head_vec[idx] = tmp;
  }
  table_vec[idx].erase(node->key);
}

void moveToTail(unsigned long idx, ListNode* node) {
  if (node == tail_vec[idx]) return;
  else if (node == head_vec[idx]) {
    removeHead(idx);
  } else {
    (node->prev)->next = node->next;
    (node->next)->prev = node->prev;
  }
  insertToTail(idx, node);
}

int access(unsigned long addr) {
  unsigned long idx = getIdx(addr);
  unsigned long tag = getTag(addr);

  auto &table = table_vec[idx];
  ListNode* node = nullptr;

  if (table.count(tag) != 0) {
    node = table[tag];
    moveToTail(idx, node);
    return 1;
  } else {
    node = new ListNode(tag, 1);
    if (table.size() == ASSOCIATIVITY) removeHead(idx);
    insertToTail(idx, node);
    return 0;
  }
}

int is_cache_miss(int load_store, unsigned long address, int cycles) {
  if (load_store == LOAD) {
    if (access(address)) {
      hits++;
      read_hits++;
      return 0;
    } else {
      misses++;
      read_misses++;
      return 1;
    }
  } else {
    if (access(address)) {
      hits++;
      return 0;
    } else {
      misses++;
      return 1;
    }
  }
}

main() {
  unsigned long address;
  long references;
  int load_store, icount;
  char marker;
  long cycles = 0;
  long base_cycles = 0;

  int i,j;


  //a good place to initialize your structures.

  printf("Cache parameters:\n");
  printf("\tCache size %d\n", CACHE_SIZE);
  printf("\tCache block size %d\n", BLOCK_SIZE);
  printf("\tCache ASSOCIATIVITY %d\n", ASSOCIATIVITY);

  // the format of the trace is
  //    # load_store address instcount
  //    where load_store is 0 (load) or 1 (store)
  //          address is the address of the memory access
  //          instcount is the number of instructions (including the load
  //            or store) between the previous access and this one.

  while (scanf("%c %d %lx %d\n",&marker,&load_store,&address,&icount) != EOF) {

    if (marker == '#')
      references++;
    else {
      printf("Oops\n");
      continue;
    }

    // for (crude) performance modeling, we will assume a base CPI of 1,
    //     thus every instruction takes one cycle, plus memory access time.
    cycles += icount;
    base_cycles += icount;
    cycles += is_cache_miss(load_store,address,cycles) * MISS_PENALTY;
  }

  printf("Simulation results:\n");
  printf("\texecution cycles %ld cycles\n",cycles);
  printf("\tinstructions %ld\n", base_cycles);
  printf("\tmemory accesses %ld\n", hits+misses);
  printf("\toverall miss rate %.2f%%\n", 100.0 * (float) misses / ((float) (hits + misses)) );
  printf("\tread miss rate %.2f%%\n", 100.0 * (float) read_misses / ((float) (read_hits + read_misses)) );
  printf("\tmemory CPI %.2f\n", (float) (cycles - base_cycles) / (float) base_cycles);
  printf("\ttotal CPI %.2f\n", (float) 1.0 + (cycles - base_cycles) / (float) base_cycles);
  printf("\taverage memory access time %.2f cycles\n",  (float) (cycles - base_cycles) / (float) (hits + misses));
  printf("load_misses %ld\n", read_misses);
  printf("store_misses %ld\n", misses - read_misses);
  printf("load_hits %ld\n", read_hits);
  printf("store_hits %ld\n", hits - read_hits);
}
