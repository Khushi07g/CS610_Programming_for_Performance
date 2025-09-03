#include <bits/stdc++.h>
using namespace std;

extern ifstream infile;
extern ofstream outfile;

extern mutex file_mutex;
extern deque<string> buffer;
extern int buffer_size;
extern int cur_buffer_size;
extern mutex buffer_mutex;
extern condition_variable not_full_cv;
extern condition_variable not_empty_cv;

extern mutex producer_mutex;
extern atomic<int> active_producers;
extern bool producers_done;

void producer_fn(int id, int Lmin, int Lmax);
void consumer_fn(int id);
