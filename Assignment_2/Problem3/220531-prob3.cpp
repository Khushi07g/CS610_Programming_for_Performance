#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>

using std::cerr;
using std::cout;
using std::endl;

using HR = std::chrono::high_resolution_clock;
using HRTimer = HR::time_point;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;

#define N (1e6)
#define NUM_THREADS (16)

// Shared variables
uint64_t var1 = 0, var2 = (N * NUM_THREADS + 1);

static inline void cpu_mfence() {
  asm volatile("mfence" ::: "memory");
}

static inline void cpu_pause() {
  asm volatile("pause" ::: "memory");
}

bool CAS(int* ptr, int expected, int desired) {
  unsigned char result;
  asm volatile (
      "lock; cmpxchgl %3,%1\n\t"
      "sete %0"
      : "=q"(result), "+m"(*ptr), "+a"(expected)
      : "r"(desired)
      : "memory");

  return result;
}

int FAI(volatile int* ptr) {
  int ret;
  asm volatile (
      "lock; xaddl %0, %1"
      : "=r"(ret), "+m"(*ptr)
      : "0"(1)
      : "memory");
  return ret;
}

// Abstract base class
class LockBase {
public:
  // Pure virtual function
  virtual void acquire(uint16_t tid) = 0;
  virtual void release(uint16_t tid) = 0;
};

typedef struct thr_args {
  uint16_t m_id;
  LockBase* m_lock;
} ThreadArgs;

/** Use pthread mutex to implement lock routines */
class PthreadMutex : public LockBase {
public:
  void acquire(uint16_t tid) override { pthread_mutex_lock(&lock); }
  void release(uint16_t tid) override { pthread_mutex_unlock(&lock); }

private:
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
};

class FilterLock : public LockBase {
  volatile int* level;
  volatile int* victim;
public:
FilterLock() : level(new volatile int[NUM_THREADS]),
               victim(new volatile int[NUM_THREADS]) {
  for (int i = 0; i < NUM_THREADS; i++) {
    level[i] = 0;
    victim[i] = -1;
  }
}
void acquire(uint16_t tid) override{
  for (int L = 1; L < NUM_THREADS; L++) {
      level[tid] = L;
      victim[L] = tid;

      cpu_mfence();

      bool wait = true;

      cpu_mfence();
      while (wait) {
        wait = false;
        for (int k = 0; k < NUM_THREADS; k++) {
          if (k == tid) continue;
          if (level[k] >= L && victim[L] == tid) {
            wait = true;
            cpu_pause();
            break;
          }
        }
      }
    }
  }
  void release(uint16_t tid) override {
    level[tid] = 0;
  }

  ~FilterLock() {
    delete[] level;
    delete[] victim;
  }
};

class BakeryLock : public LockBase {
  volatile int* label;
  volatile bool* choosing;
public:
  BakeryLock() : choosing(new volatile bool[NUM_THREADS]),
                 label(new volatile int[NUM_THREADS]) {
    for (int i = 0; i < NUM_THREADS; i++) {
      choosing[i] = false;
      label[i] = 0;
    }
  }
  void acquire(uint16_t tid) override {
    choosing[tid] = true;
    cpu_mfence();

    int max_label = 0;
    for (int j = 0; j < NUM_THREADS; j++) {
      int lbl = label[j];
      if (lbl > max_label) {
        max_label = lbl;
      }
    }
    label[tid] = max_label + 1;
    cpu_mfence();

    for (int j = 0; j < NUM_THREADS; j++) {
      if (j == tid) continue;
      while (choosing[j] != false && (label[j] < label[tid] || (label[j] == label[tid] && j < tid))) {
        cpu_pause();
      }
    }
  }
  void release(uint16_t tid) override {
    choosing[tid] = false;
  }

  ~BakeryLock() {
    delete[] choosing;
    delete[] label;
  }
};

class SpinLock : public LockBase {
  volatile int lock_flag;
public:
  SpinLock() : lock_flag(0) {}
  ~SpinLock() {}
  void acquire(uint16_t tid) override {
    while (CAS((int*)&lock_flag, 0, 1) == false) {
      cpu_pause();
    }
  }
  void release(uint16_t tid) override {
    lock_flag = 0;
  }

};

class TicketLock : public LockBase {
  volatile int next_ticket;
  volatile int serving;
public:
  TicketLock() : next_ticket(0), serving(0) {}
  void acquire(uint16_t tid) override {
    int my_ticket = FAI((int*)&next_ticket);
    cpu_mfence();
    while (serving != my_ticket) {
      cpu_pause();
    }
  }
  void release(uint16_t tid) override {
    serving++;
  }
  ~TicketLock() {}
};

class ArrayQLock : public LockBase {
  struct alignas(64) PaddedFlag {
    volatile bool value;
    char padding[64 - sizeof(bool)];
  };
  volatile int tail;
  volatile PaddedFlag* flag;
  static thread_local int my_slot;
  public:
  ArrayQLock() : tail(0), flag(new volatile PaddedFlag[NUM_THREADS]()) {
    for (int i = 0; i < NUM_THREADS; i++) {
      flag[i].value = false;
    }
    flag[0].value = true;
  }
  void acquire(uint16_t tid) override {
    int slot = FAI((int*)&tail) % NUM_THREADS;
    my_slot = slot;
    while (!flag[slot].value) {
      cpu_pause();
    }
  }
  void release(uint16_t tid) override {
    int slot = my_slot;
    flag[slot].value = false;
    flag[(slot + 1) % NUM_THREADS].value = true;
  }

  ~ArrayQLock() {
    delete[] flag;
  }
};

thread_local int ArrayQLock::my_slot = -1;

/** Estimate the time taken */
std::atomic_uint64_t sync_time = 0;

inline void critical_section() {
  var1++;
  var2--;
}

/** Sync threads at the start to maximize contention */
pthread_barrier_t g_barrier;

void* thrBody(void* arguments) {
  ThreadArgs* tmp = static_cast<ThreadArgs*>(arguments);
  if (false) {
    cout << "Thread id: " << tmp->m_id << " starting\n";
  }

  // Wait for all other producer threads to launch before proceeding.
  pthread_barrier_wait(&g_barrier);

  HRTimer start = HR::now();
  for (int i = 0; i < N; i++) {
    tmp->m_lock->acquire(tmp->m_id);
    critical_section();
    tmp->m_lock->release(tmp->m_id);
  }
  HRTimer end = HR::now();
  auto duration = duration_cast<milliseconds>(end - start).count();

  // A barrier is not required here
  sync_time.fetch_add(duration);
  pthread_exit(NULL);
}

int main() {
  int error = pthread_barrier_init(&g_barrier, NULL, NUM_THREADS);
  if (error != 0) {
    cerr << "Error in barrier init.\n";
    exit(EXIT_FAILURE);
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_t tid[NUM_THREADS];
  ThreadArgs args[NUM_THREADS] = {{0}};

  // Pthread mutex
  LockBase* lock_obj = new PthreadMutex();
  uint16_t i = 0;
  while (i < NUM_THREADS) {
    args[i].m_id = i;
    args[i].m_lock = lock_obj;

    error = pthread_create(&tid[i], &attr, thrBody, (void*)(args + i));
    if (error != 0) {
      cerr << "\nThread cannot be created : " << strerror(error) << "\n";
      exit(EXIT_FAILURE);
    }
    i++;
  }

  i = 0;
  void* status;
  while (i < NUM_THREADS) {
    error = pthread_join(tid[i], &status);
    if (error) {
      cerr << "ERROR: return code from pthread_join() is " << error << "\n";
      exit(EXIT_FAILURE);
    }
    i++;
  }

  assert(var1 == N * NUM_THREADS && var2 == 1);
  // cout << "Var1: " << var1 << "\tVar2: " << var2 << "\n";
  cout << "Pthread mutex: Time taken (us): " << sync_time << "\n";

  // Filter lock
  var1 = 0;
  var2 = (N * NUM_THREADS + 1);
  sync_time.store(0);

  lock_obj = new FilterLock();
  i = 0;
  while (i < NUM_THREADS) {
    args[i].m_id = i;
    args[i].m_lock = lock_obj;

    error = pthread_create(&tid[i], &attr, thrBody, (void*)(args + i));
    if (error != 0) {
      printf("\nThread cannot be created : [%s]", strerror(error));
      exit(EXIT_FAILURE);
    }
    i++;
  }

  i = 0;
  while (i < NUM_THREADS) {
    error = pthread_join(tid[i], &status);
    if (error) {
      printf("ERROR: return code from pthread_join() is %d\n", error);
      exit(EXIT_FAILURE);
    }
    i++;
  }

  cout << "Var1: " << var1 << "\tVar2: " << var2 << "\n";
  assert(var1 == N * NUM_THREADS && var2 == 1);
  cout << "Filter lock: Time taken (us): " << sync_time << "\n";

  // Bakery lock
  var1 = 0;
  var2 = (N * NUM_THREADS + 1);
  sync_time.store(0);

  lock_obj = new BakeryLock();
  i = 0;
  while (i < NUM_THREADS) {
    args[i].m_id = i;
    args[i].m_lock = lock_obj;

    error = pthread_create(&tid[i], &attr, thrBody, (void*)(args + i));
    if (error != 0) {
      printf("\nThread cannot be created : [%s]", strerror(error));
      exit(EXIT_FAILURE);
    }
    i++;
  }

  i = 0;
  while (i < NUM_THREADS) {
    error = pthread_join(tid[i], &status);
    if (error) {
      printf("ERROR: return code from pthread_join() is %d\n", error);
      exit(EXIT_FAILURE);
    }
    i++;
  }

  cout << "Var1: " << var1 << "\tVar2: " << var2 << "\n";
  assert(var1 == N * NUM_THREADS && var2 == 1);
  cout << "Bakery lock: Time taken (us): " << sync_time << "\n";

  // Spin lock
  var1 = 0;
  var2 = (N * NUM_THREADS + 1);
  sync_time.store(0);

  lock_obj = new SpinLock();
  i = 0;
  while (i < NUM_THREADS) {
    args[i].m_id = i;
    args[i].m_lock = lock_obj;

    error = pthread_create(&tid[i], &attr, thrBody, (void*)(args + i));
    if (error != 0) {
      printf("\nThread cannot be created : [%s]", strerror(error));
      exit(EXIT_FAILURE);
    }
    i++;
  }

  i = 0;
  while (i < NUM_THREADS) {
    error = pthread_join(tid[i], &status);
    if (error) {
      printf("ERROR: return code from pthread_join() is %d\n", error);
      exit(EXIT_FAILURE);
    }
    i++;
  }

  cout << "Var1: " << var1 << "\tVar2: " << var2 << "\n";
  assert(var1 == N * NUM_THREADS && var2 == 1);
  cout << "Spin lock: Time taken (us): " << sync_time << "\n";

  // Ticket lock
  var1 = 0;
  var2 = (N * NUM_THREADS + 1);
  sync_time.store(0);

  lock_obj = new TicketLock();
  i = 0;
  while (i < NUM_THREADS) {
    args[i].m_id = i;
    args[i].m_lock = lock_obj;

    error = pthread_create(&tid[i], &attr, thrBody, (void*)(args + i));
    if (error != 0) {
      printf("\nThread cannot be created : [%s]", strerror(error));
      exit(EXIT_FAILURE);
    }
    i++;
  }

  i = 0;
  while (i < NUM_THREADS) {
    error = pthread_join(tid[i], &status);
    if (error) {
      printf("ERROR: return code from pthread_join() is %d\n", error);
      exit(EXIT_FAILURE);
    }
    i++;
  }

  cout << "Var1: " << var1 << "\tVar2: " << var2 << "\n";
  assert(var1 == N * NUM_THREADS && var2 == 1);
  cout << "Ticket lock: Time taken (us): " << sync_time << "\n";

  // Array Q lock
  var1 = 0;
  var2 = (N * NUM_THREADS + 1);
  sync_time.store(0);

  lock_obj = new ArrayQLock();
  i = 0;
  while (i < NUM_THREADS) {
    args[i].m_id = i;
    args[i].m_lock = lock_obj;

    error = pthread_create(&tid[i], &attr, thrBody, (void*)(args + i));
    if (error != 0) {
      printf("\nThread cannot be created : [%s]", strerror(error));
      exit(EXIT_FAILURE);
    }
    i++;
  }

  i = 0;
  while (i < NUM_THREADS) {
    error = pthread_join(tid[i], &status);
    if (error) {
      printf("ERROR: return code from pthread_join() is %d\n", error);
      exit(EXIT_FAILURE);
    }
    i++;
  }

  cout << "Var1: " << var1 << "\tVar2: " << var2 << "\n";
  assert(var1 == N * NUM_THREADS && var2 == 1);
  cout << "Array Q lock: Time taken (us): " << sync_time << "\n";

  pthread_barrier_destroy(&g_barrier);
  pthread_attr_destroy(&attr);

  pthread_exit(NULL);
}
