#include <bits/stdc++.h>
#include <pthread.h>

using namespace std;

#define Max_Nr_Characters 100
#define File_Name_Length 10

typedef struct {
    int file_size;
    char *file_name;
} File_data;

struct CompareFilesSize {
    bool operator()(File_data const& p1, File_data const& p2)
    {
        return p1.file_size < p2.file_size;
    }
};

typedef struct {
    int index;
    int nr_mapperi;
    int nr_reduceri;
    pthread_mutex_t mutex;
    pthread_barrier_t *barrier;
    multimap<int, unsigned long long> *vector_puteri_perfecte;
    priority_queue<File_data, vector<File_data>, CompareFilesSize> *queue_files;
} Mapper_Reducer;


