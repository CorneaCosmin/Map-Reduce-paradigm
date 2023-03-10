#include "tema1.h"

void *Map_Reduce(void *arg) {
    Mapper_Reducer thread_mapper_reducer = *(Mapper_Reducer*) arg;    
    //verific daca threadul are indexul unui mapper
    if (thread_mapper_reducer.index < thread_mapper_reducer.nr_mapperi) {
        int i, j;
        int nr_valori;
        unsigned long long putere_perfecta;
        //procesez fisiere cat timp coada de fisiere nu este goala
        while (!thread_mapper_reducer.queue_files->empty()) {
            File_data current_file;
            //scot primul fisier din coada si il deschid pentru citire
            //folosesc un mutex pentru a nu accesa mai multe threaduri
            //aceeasi zona de memorie
            pthread_mutex_lock(&thread_mapper_reducer.mutex);
            if (!thread_mapper_reducer.queue_files->empty()) {
                current_file = thread_mapper_reducer.queue_files->top();
                thread_mapper_reducer.queue_files->pop();
            }
            pthread_mutex_unlock(&thread_mapper_reducer.mutex);
            FILE *fisier_input = fopen(current_file.file_name, "r");

            //verific daca pot deschide fisierul
            if (fisier_input == NULL) {
                printf("Cannot open file!");
                exit(1);
            }

            //citesc valorile din fiecare fisier de puteri perfecte si le adaug
            //impreuna cu exponentii corespunzatori la mapul cu indicele threadului 
            //din vectorul de mapuri
            fscanf(fisier_input, "%d", &nr_valori);
            for (i = 0; i < nr_valori; i++) {
                fscanf(fisier_input, "%llu", &putere_perfecta);
                //fac o cautare binara pentru a verifica daca un numar este
                //putere perfecta si pentru a gasi exponentii corespunzatori
                for (j = 2; j <= thread_mapper_reducer.nr_reduceri + 1; j++) {
                    long long int left = 1;
                    long long int right = putere_perfecta;
                    long long int mid;
                    while (right - left >= 0) {
                        mid = (right + left) / 2;
                        if (pow(mid, j) == putere_perfecta) {
                            thread_mapper_reducer.vector_puteri_perfecte[thread_mapper_reducer.index].insert(pair <int, unsigned long long int> (j, putere_perfecta));
                            break;
                        } else if(pow(mid, j) > putere_perfecta) {
                            right = mid - 1;
                        } else if(pow(mid, j) < putere_perfecta) {
                            left = mid + 1;
                        }
                    }
                }
            }
            fclose(fisier_input); 
        }   
    }

    //astept toate threadurile pentru a avea toate operatiile de Map efectuate
    pthread_barrier_wait(thread_mapper_reducer.barrier);

    //verific daca threadul are indexul unui reducer
    if (thread_mapper_reducer.index >= thread_mapper_reducer.nr_mapperi) {
        set<unsigned long long> puteri_perfecte_unice;
        int i;
        //calculez exponentul pentru care threadul face Reduce si parcurg vectorul de mapuri
        //pentru a retine puterile perfecte cu acel exponent
        int exponent = thread_mapper_reducer.index - thread_mapper_reducer.nr_mapperi + 2;
        for (i = 0; i < thread_mapper_reducer.nr_mapperi; i++) {
            map<int, unsigned long long>::iterator itr;
            for (itr = thread_mapper_reducer.vector_puteri_perfecte[i].begin(); 
                    itr != thread_mapper_reducer.vector_puteri_perfecte[i].end(); itr++) {
                if (itr->first == exponent) {
                    puteri_perfecte_unice.insert(itr->second);
                }
            }
        }

        //formez numele fisierului de output corespunzator exponentului
        char *file_name = (char*) malloc(File_Name_Length * sizeof(char));
        strcpy(file_name, "out");
        string exponent_to_char = to_string(exponent);
        char const *exponent_char = exponent_to_char.c_str();
        strcat(file_name, exponent_char);
        strcat(file_name, ".txt");

        //deschid fisierul de iesire pentru scriere si verific daca primesc eroare
        FILE *output_file = fopen(file_name, "w");
        if(output_file == NULL) {
            printf("Error!");   
            exit(1);             
        }
        //scriu numarul de elemente din setul pentru fiecare putere si inchid fisierul
        fprintf(output_file, "%ld", puteri_perfecte_unice.size());
        free(file_name);
        fclose(output_file);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    //declar variabilele
    int nr_mapperi, nr_reduceri, nr_fisiere_input;
    int i, r;
    void *status; 
    multimap<int, unsigned long long> *vector_puteri_perfecte;
    File_data *fisiere_citire;
    priority_queue<File_data, vector<File_data>, CompareFilesSize> queue_files;
	pthread_t *threads_mapper_reducer;
    pthread_mutex_t mutex;
    pthread_barrier_t barrier;

    //retin argumentele rularii
    nr_mapperi = atoi(argv[1]);
    nr_reduceri =  atoi(argv[2]);
    FILE *fisier_intrare = fopen(argv[3], "r");

    //verific daca pot deschide fisierul
    if (fisier_intrare == NULL) {
       printf("Cannot open file!");
       exit(1);
    }

    //initializari variabile
    vector_puteri_perfecte = new multimap<int, unsigned long long>[nr_mapperi + 1];
    threads_mapper_reducer = new pthread_t[nr_mapperi + nr_reduceri];
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, nr_mapperi + nr_reduceri);

    //citesc si retin din fisierul de intrare numarul si numele fisierelor de input
    fscanf(fisier_intrare, "%d", &nr_fisiere_input);
    fisiere_citire = new File_data[nr_fisiere_input + 1];
    for (i = 0; i < nr_fisiere_input; i++) {
        fisiere_citire[i].file_name = (char*) malloc(Max_Nr_Characters * sizeof(char));
        fscanf(fisier_intrare, "%s", fisiere_citire[i].file_name);
        //calculez dimensiunea fisierelor de input
        ifstream in_file(fisiere_citire[i].file_name, ios::binary);
        in_file.seekg(0, ios::end);
        int file_size = in_file.tellg();
        fisiere_citire[i].file_size = file_size;
    }

    //adaug fisierele intr-o coada de prioritati dupa dimensiune
    for (i = 0; i < nr_fisiere_input; i++) {
        queue_files.push(fisiere_citire[i]);
    }

    //deschid nr_mapperi + nr_reduceri threaduri pentru a face operatiile de Map si Reduce
    for (i = 0; i < nr_mapperi + nr_reduceri; i++) {
        Mapper_Reducer *thread_mapper_reducer = new Mapper_Reducer;
        thread_mapper_reducer->index = i;
        thread_mapper_reducer->vector_puteri_perfecte = vector_puteri_perfecte;
        thread_mapper_reducer->queue_files = &queue_files;
        thread_mapper_reducer->mutex = mutex;
        thread_mapper_reducer->barrier = &barrier;
        thread_mapper_reducer->nr_mapperi = nr_mapperi;
        thread_mapper_reducer->nr_reduceri = nr_reduceri;
		r = pthread_create(&threads_mapper_reducer[i], NULL, Map_Reduce, thread_mapper_reducer);
		if (r) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

    //apelez functia join pentru a astepta toate threadurile    
	for (i = 0; i < nr_mapperi + nr_reduceri; i++) {
		r = pthread_join(threads_mapper_reducer[i], &status);
		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}

    //dezaloc memoria alocata variabilelor
    for (i = 0; i < nr_fisiere_input; i++) {
        free(fisiere_citire[i].file_name);
    }
    delete[] vector_puteri_perfecte;
    delete[] fisiere_citire;
    delete[] threads_mapper_reducer;

    //dezaloc mutexul si bariera
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    return 0;
}

