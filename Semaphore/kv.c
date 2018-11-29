#include <stdio.h>
#include <stdlib.h>
// semaphore related import
#include <fcntl.h>
#include <semaphore.h>
// read & write related import
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_KEY_LEN 32
#define MAX_VAL_LEN 128
#define MAX_POD_LEN 256
#define MAX_TABLE_SIZE 50 // maximum number ofkey/value pair to be stored// semaphore store
#define __TEST_SHARED_MEM_NAME__ "/GTX_1080_TI"
#define __TEST_SHARED_SEM_NAME__ "/ONLY_TEARS"

/*
Compile with 

gcc kv.c -pthread -lrt -w (link with pthread, link necessary libraries)
run with ./a.out

please see test.log to see all testing

@author james tang

*/

sem_t *store;

// size of table
int BUFF_SIZE = MAX_TABLE_SIZE;
char *BUFF_SHM = "BUFF";
int shm_fd;
void *base_addr;

// create a custom data structure called tbl that stores key-value pair
struct KV{
	int index;
	int sequence;
	char key[MAX_KEY_LEN];
	char *value[MAX_POD_LEN];
};

int kv_store_create(char *name){
	int store_size = sizeof(struct KV) * BUFF_SIZE;
	store = sem_open(name,O_CREAT,0777,1);

	if(store == SEM_FAILED){
		printf("Semaphore creation failed\n");
		sem_close(store);
		return -1;
	}
	else{
		printf("Semaphore creation success\n");
	}

    //Open shared mem
    shm_fd = shm_open(BUFF_SHM, O_CREAT | O_RDWR, 0777);

    if (shm_fd == -1)
    {
        printf("Shared memory creation failed\n");
        
    }else{
    	printf("Shared memory creation success\n");
    }

    //Truncate the memory into right size
    ftruncate(shm_fd, store_size);
    base_addr = mmap(NULL, store_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (base_addr == MAP_FAILED)
    {
        printf("Map failed\n");
        shm_unlink(BUFF_SHM);
		return -1;
	}

	return 0;	
}

// initialize the kv
void init_kv(struct KV *base){
	sem_wait(store);
	for (int i=0;i<BUFF_SIZE;i++){
		(base+i) -> index = i;
		(base+i) -> sequence = 0;
		strcpy((base+i)->key,"");
		strcpy((base+i)->value,"");
	}
	sem_post(store);
	return;
}

// helper function to print the entire table
void print_kv(struct KV *base){
	sem_wait(store);
	printf("The KV Table has following data:\n");
	for (int i=0;i<BUFF_SIZE;i++){
		printf("%s %d","Index:",(base+i)->index);
		printf("%s %s"," | Key: ",(base+i)->key);
		printf("%s %s\n"," | Value: ",(base+i)->value);
	}
	sem_post(store);
	return;
}

// write to the table with FIFO
int kv_store_write(struct KV *base,char *key, char *value){
	// if key and value is longer than 32/256, then truncate before write to kv
	if(strlen(key)>MAX_KEY_LEN){
		printf("%s\n","Key is too long, will be truncated");
		char temp[MAX_KEY_LEN];
		strncpy(temp,key,MAX_KEY_LEN);
		printf("%s",temp);
		strncpy(key,temp,MAX_VAL_LEN);
	}

	if(strlen(value)>MAX_VAL_LEN){
		printf("%s\n","Value is too long, will be truncated");
		strncpy(value,value,MAX_VAL_LEN);
	}
	
	sem_wait(store);

	// marker to know if the table is full
	int is_updated = 0;

	for (int i=0;i<MAX_TABLE_SIZE;i++){
		// duplicate record
		if(strcmp((base+i)->key,key)==0){
			char new_val[200]="";
			strcat(new_val,(base+i)->value);
			strcat(new_val," ");
			strcat(new_val,value);
			strcpy((base+i)->value,new_val);
			is_updated = 1;
			sem_post(store);
			return 0;
		}
		// if the key at index is empty, then insert
		else if(strcmp((base+i)->key,"")==0){
			strcpy((base+i)->key,key);
			strcpy((base+i)->value,value);
			is_updated = 1;
			sem_post(store);
			return 0;
		}
	}
	// the kv table is full, use FIFO, removethe earliest entry
	if(is_updated==0){
		strcpy((base+0)->key,key);
		strcpy((base+0)->value,value);
	}
	sem_post(store);
	return 0;
}

// sequential read value from the pod
char *kv_store_read_all(struct KV *base,char *key){
	sem_wait(store);

	for (int i=0;i<BUFF_SIZE;i++){
		// if a key is found that match the input key
		if(strcmp((base+i)->key,key)==0){
			sem_post(store);
			return (base+i)->value;
		}
	}
	sem_post(store);
	return NULL;
}

// reads all values from the table 
char *kv_store_read(struct KV *base,char *key){
	sem_wait(store);
	for (int i=0;i<BUFF_SIZE;i++){
		// if a key is found that match the input key
		if(strcmp((base+i)->key,key)==0){
			// find the num of values in pod
			int length_of_tok = 1;
			char ptr[MAX_VAL_LEN];
			strcpy(ptr,(base+i)->value);
			for(int i=0;i<strlen(ptr);i++){
				if(ptr[i]== ' '){
					length_of_tok++;
				}
			}
			// if there is only one entry in the pod
			if(length_of_tok==1){
				sem_post(store);
				return (base+i)->value;
			}else{
				char *toklist;
				char value[MAX_VAL_LEN];
				strcpy(value,(base+i)->value);
				toklist = strtok(value," ");
				// reset pointer if all data in pod is read
				if((base+i)->sequence >=length_of_tok){
					(base+i)->sequence = 0;
				}

				int count = 0;
				while(toklist!=NULL){
					// look for the next value in the pod
					if(count==(base+i)->sequence){
						(base+i)->sequence = (base+i)->sequence +1;
						sem_post(store);
                        //printf("tok:%s\n",toklist);
						return toklist;
					}else{
						count++;
					}
					toklist = strtok(NULL," ");
				}
			}
			
		}
	}
	sem_post(store);
	return NULL;
}


int main(){
	// random number initialization
	srand(time(NULL));
	// remove sem memory before testing
	shm_unlink(__TEST_SHARED_MEM_NAME__);

	int errors = 0;
    char *temp;
    char **temp_all;
    char ***data_buf;
    char **keys_buf;
    int elem_num[MAX_TABLE_SIZE];
    int expected_result[MAX_TABLE_SIZE*2][MAX_TABLE_SIZE];
    int patterns[MAX_TABLE_SIZE*2];
    int pattern_length = 0;

    srand(time(NULL));
    data_buf = calloc(1, sizeof(char **) * MAX_TABLE_SIZE);
    keys_buf = calloc(1, sizeof(char **) * MAX_TABLE_SIZE);
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        data_buf[i] = calloc(1, sizeof(char*) * MAX_TABLE_SIZE*2);
        keys_buf[i] = calloc(1, sizeof(char*) * MAX_TABLE_SIZE);
        for(int j = 0; j < MAX_TABLE_SIZE*2; j++){
            data_buf[i][j] = calloc(1, sizeof(char) * MAX_VAL_LEN+ 1);
        }
    }

    printf("-----------One Threaded Testing Of Shared Memory Database-----------\n");
    printf("-----------Initializing DB-----------\n");
    kv_store_create(__TEST_SHARED_MEM_NAME__);
    init_kv(base_addr);
    printf("-----------Attempting Invalid Read-----------\n");

    temp = kv_store_read(base_addr,"gtx 1080ti!!!");
    if(temp != NULL){
        errors++;
        printf("Did not return null on invalid key. \n");
        free(temp);
    }else{
    	printf("%s %s\n","Invalid read is good for test case: ","gtx 1080ti!!!");
    }
    temp_all = kv_store_read_all(base_addr,"need that ryzen");
    if(temp_all != NULL){
        errors++;
        printf("Did not return null on invalid key. \n");
        free(temp_all);
    }else{
    	printf("%s %s\n","Invalid read is good for test case: ","need that ryzen");
    }

    printf("-----------Attempting Simple Read and Writes-----------\n");
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        read_write_test(keys_buf, data_buf, elem_num, i, &errors);
    }
    printf("           Error Count: %d\n", errors);

    printf("-----------Created KV Table-----------\n");
    print_kv(base_addr);

    printf("-----------Attempting Write----------\n");
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        write_test(keys_buf, data_buf, elem_num, i, &errors);
    }
    printf("           Total Error Count: %d\n", errors);
    print_kv(base_addr);
   
    printf("-----------Attempting Write for Multiple Data Under One Key----------\n");
    int error = write_test_dup(base_addr);
    printf("           Total Error Count: %d\n", error);
    
    printf("-----------Attempting Write Using FIFO----------\n");
    error = fifo(base_addr);
    printf("           Total Error Count: %d\n", error);
   
    printf("-----------Attempting Read----------\n");
    error = read_test();
    printf("           Total Error Count: %d\n", errors);

    printf("-----------Attempting Read_all----------\n");
    error = read_all_test();
    printf("           Total Error Count: %d\n", errors);
    
}
/* 
This test the KV table's ability to perform insert for ssimple read
*/
void read_write_test(char **keys_buf, char ***data_buf, int elem_num[MAX_TABLE_SIZE], int i, int *errors){
    int temp_flag = -1;
    char *temp;
    generate_string(data_buf[i][elem_num[i]], MAX_VAL_LEN);
    generate_key(keys_buf[i],MAX_KEY_LEN, keys_buf, i+1);

    temp_flag = kv_store_write(base_addr,keys_buf[i], data_buf[i][elem_num[i]]);
    temp = kv_store_read(base_addr,keys_buf[i]);
    if(temp == NULL || temp_flag != 0){
        printf("Gave NULL when value should be there or write failed\n");
        errors++;
    }else if(strcmp(temp, data_buf[i][elem_num[i]]) != 0){
        printf("Read : %s\nShould have read %s\n", temp, data_buf[i][elem_num[i]]);
        errors++;
    }
    elem_num[i]++;
}
/* 
This test the KV table's ability to perform  simple write
*/
void write_test(char **keys_buf, char ***data_buf, int elem_num[MAX_TABLE_SIZE], int i, int *errors){
    int temp_flag = -1;
    char *temp;
    generate_string(data_buf[i][elem_num[i]], MAX_VAL_LEN);
    generate_key(keys_buf[i],MAX_KEY_LEN, keys_buf, i+1);

    temp_flag = kv_store_write(base_addr,keys_buf[i], data_buf[i][elem_num[i]]);
   
    if(temp == NULL || temp_flag != 0){
        printf("Gave NULL when value should be there or write failed\n");
        errors++;
    }
    elem_num[i]++;
}
/* 
This test the KV table's ability to perform insert for duplicate keys
*/
int write_test_dup(){
    shm_unlink(__TEST_SHARED_MEM_NAME__);
    kv_store_create(__TEST_SHARED_MEM_NAME__);
    init_kv(base_addr);

    char keys[MAX_TABLE_SIZE][32];
    int dup_error = 0;
    // create KV table with one set of key/value
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char key_string [32];
        generate_string(key_string,32);
        // add key to keychain
        strcpy(keys[i],key_string);
        // generate a random number between 0 and 50
        r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);

        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,keys[i],value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
        } 
    }
    printf("-----------Before Inserting Dup Keys----------\n");
    print_kv(base_addr);

    // attempt to add to each key another data
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);
        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,keys[i],value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
            dup_error++;
        } 
    }
    printf("-----------After Inserting Dup Keys----------\n");
    print_kv(base_addr);
    return dup_error;

}
/* 
This test the KV table's ability to replace table with new data using FIFO when the table is full
*/
int fifo(){
    shm_unlink(__TEST_SHARED_MEM_NAME__);
    kv_store_create(__TEST_SHARED_MEM_NAME__);
    init_kv(base_addr);
    
    int fifo_error = 0;
    // create KV table with one set of key/value
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char key_string [32];
        generate_string(key_string,32);
        // generate a random number between 0 and 50
        r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);

        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,key_string,value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
        } 
    }
    printf("-----------Before FIFO Insert---------\n");
    print_kv(base_addr);

    // attempt to add to 10 other key/data pair
    for(int i = 0; i < 10; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char key_string [32];
        generate_string(key_string,32);
        // generate a random number between 0 and 50
        r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);

        temp_flag = kv_store_write(base_addr,key_string,value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
            fifo_error++;
        } 
    }
    
    printf("-----------After FIFO Insert---------\n");
    print_kv(base_addr);
    return fifo_error;

}

void read_test(){
    shm_unlink(__TEST_SHARED_MEM_NAME__);
    kv_store_create(__TEST_SHARED_MEM_NAME__);
    init_kv(base_addr);

    char keys[MAX_TABLE_SIZE][32];
    char values[MAX_TABLE_SIZE][32];
    int read_error = 0;
    // create KV table with one set of key/value
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char key_string [32];
        generate_string(key_string,32);
        // add key to keychain
        strcpy(keys[i],key_string);
        // generate a random number between 0 and 50
        r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);
        strcpy(values[i],value_string);
        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,keys[i],value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
        } 
    }

    // attempt to add to each key another data
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);
        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,keys[i],value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
        } 
    }
    print_kv(base_addr);
    
    // read using keys
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        kv_store_read(base_addr,keys[i]);
        printf("For index %d Second read access:%s\n",i,kv_store_read(base_addr,keys[i]));
    }
    
}

void read_all_test(){
    shm_unlink(__TEST_SHARED_MEM_NAME__);
    kv_store_create(__TEST_SHARED_MEM_NAME__);
    init_kv(base_addr);

    char keys[MAX_TABLE_SIZE][32];
    char values[MAX_TABLE_SIZE][32];
    int read_error = 0;
    // create KV table with one set of key/value
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char key_string [32];
        generate_string(key_string,32);
        // add key to keychain
        strcpy(keys[i],key_string);
        // generate a random number between 0 and 50
        r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);
        strcpy(values[i],value_string);
        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,keys[i],value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
        } 
    }

    // attempt to add to each key another data
    for(int i = 0; i < MAX_TABLE_SIZE; i++){
        int temp_flag = -1;
        // generate a random number between 0 and 50
        int r = rand() %(50 + 1 - 0) + 0;
        char value_string [32];
        // pull a random data and from a new key/data pair
        generate_string(value_string,32);
        // pull a random data and from a new key/data pair
        temp_flag = kv_store_write(base_addr,keys[i],value_string);

        if(temp_flag != 0){
            printf("Gave NULL when value should be there or write failed\n");
        } 
    }
    print_kv(base_addr);
    
    // read using keys
    for(int i = 0; i < MAX_TABLE_SIZE; i++){

        printf("For index %d read access all: %s\n",i,kv_store_read_all(base_addr,keys[i]));
    }
    
}

void generate_string(char buf[], int length){
    int type;
    for(int i = 0; i < length; i++){
        type = rand() % 3;
        if(type == 2)
            buf[i] = rand() % 26 + 'a';
        else if(type == 1)
            buf[i] = rand() % 10 + '0';
        else
            buf[i] = rand() % 26 + 'A';
    }
    buf[length - 1] = '\0';
}

void generate_unique_data(char buf[], int length, char **keys_buf, int num_keys){
    generate_string(buf, MAX_VAL_LEN);
    int counter = 0;
    for(int i = 0; i < num_keys; i++){
        if(strcmp(keys_buf[i], buf) == 0){
            counter++;
        }
    }
    if(counter > 1){
        generate_unique_data(buf, length, keys_buf, num_keys);
    }
    return;
}

void generate_key(char buf[], int length, char **keys_buf, int num_keys){
    generate_string(buf, length);
    int counter = 0;
    for(int i = 0; i < num_keys; i++){
        if(strcmp(keys_buf[i], buf) == 0){
            counter++;
        }
    }
    if(counter > 1){
        generate_key(buf, length, keys_buf, num_keys);
    }
    return;
}