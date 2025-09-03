#include "220531.h"

ifstream infile;    
ofstream outfile;

mutex file_mutex;

deque<string> buffer;
int buffer_size = 0;
int cur_buffer_size = 0;
mutex buffer_mutex;
condition_variable not_full_cv;
condition_variable not_empty_cv;

mutex producer_mutex;

atomic<int> active_producers{0};
bool producers_done = false;

void producer_fn(int id, int Lmin, int Lmax)
{
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(Lmin, Lmax);

    while (true)
    {
        int L = dist(rng);
        vector<string> lines;
        {
            lock_guard<mutex> lg(file_mutex);
            string line;
            for (int i = 0; i < L; ++i)
            {
                if (!std::getline(infile, line)) break;
                lines.push_back(line);
            }
        }

        if (lines.empty())
        {
            break;
        }

        unique_lock<mutex> prod_lock(producer_mutex);

        int rem = (int)lines.size();
        int start_idx = 0;
        while (rem > 0)
        {
            int segment = min(rem, buffer_size);

            unique_lock<mutex> buf_lock(buffer_mutex);
            not_full_cv.wait(buf_lock, [&]{ return (cur_buffer_size + segment <= buffer_size); });
            for (int k = 0; k < segment; ++k)
            {
                buffer.push_back(lines[start_idx + k]);
            }
            cur_buffer_size += segment;

            buf_lock.unlock();
            not_empty_cv.notify_one();

            start_idx += segment;
            rem -= segment;
        }
    }
    if (active_producers.fetch_sub(1) == 1)
    {
        lock_guard<mutex> lg(buffer_mutex);
        producers_done = true;
        not_empty_cv.notify_all();
    }
}

void consumer_fn(int id)
{
    while (true)
    {
        std::unique_lock<std::mutex> buf_lock(buffer_mutex);
        not_empty_cv.wait(buf_lock, [&] { return (!buffer.empty()) || producers_done; });

        if (buffer.empty())
        {
            if (producers_done)
            {
                break;
            }
            else
            {
                continue;
            }
        }

        while (!buffer.empty())
        {
            string ln = buffer.front();
            outfile << ln << '\n';
            buffer.pop_front();
        }

        cur_buffer_size = 0;
        buf_lock.unlock();
        not_full_cv.notify_all();
    }
}

int main(int argc, char **argv)
{
    if (argc != 7)
    {
        cerr << "Usage: " << argv[0] << " <input_path R> <T> <Lmin> <Lmax> <M> <output_path W>\n";
        return 1;
    }

    string input_path = argv[1];
    int T, Lmin, Lmax, M;
    try
    {
        T = stoi(argv[2]);
        Lmin = stoi(argv[3]);
        Lmax = stoi(argv[4]);
        M = stoi(argv[5]);
    }
    catch (const invalid_argument &e)
    {
        cerr << "Invalid argument: " << e.what() << "\n";
        return 1;
    }

    string output_path = argv[6];

    buffer_size = M;

    infile.open(input_path);
    if (!infile.is_open())
    {
        cerr << "Failed to open input file: " << input_path << "\n";
        return 1;
    }
    outfile.open(output_path);
    if (!outfile.is_open())
    {
        cerr << "Failed to open output file: " << output_path << "\n";
        return 1;
    }

    active_producers = T;
    vector<thread> producers;
    for (int i = 0; i < T; ++i)
    {
        producers.emplace_back(producer_fn, i, Lmin, Lmax);
    }

    int C = max(1, T / 2);
    vector<thread> consumers;
    for (int i = 0; i < C; ++i)
    {
        consumers.emplace_back(consumer_fn, i);
    }

    for (auto &th : producers)
    {
        if (th.joinable()) th.join();
    }

    for (auto &th : consumers)
    {
        if (th.joinable()) th.join();
    }

    infile.close();
    outfile.close();

    return 0;
}
