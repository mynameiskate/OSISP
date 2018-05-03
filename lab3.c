/*
3. Написать программу поиска одинаковых по содержимому файлов в двух каталогах, например, Dir1 и Dir2. Пользователь задаёт имена Dir1 и Dir2. В результате работы программы файлы, имеющиеся в Dir1, сравниваются с файлами в Dir2 по их содержимому. Процедуры сравнения должны запускаться в отдельном процессе для каждой пары сравниваемых файлов. Каждый процесс выводит на экран свой pid, имя файла, общее число просмотренных байт и результаты сравнения. Число одновременно работающих процессов не должно превышать N (вводится пользователем). Скопировать несколько файлов из каталога /etc в свой домашний каталог. Проверить работу программы для каталога /etc и домашнего каталога.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/wait.h>

int process_limit = 0;
int process_count = 0;

typedef struct FileInfo {
	long int size;
	char *name;
} FileInfo;

typedef struct FileNode {
	FileInfo info;
	struct FileNode *next;	
} FileNode;

void add_node(FileNode **head, FileInfo node) {
	FileNode *temp = (FileNode *)malloc(sizeof(FileNode));
	temp->info = node;
	temp->next = (*head);
	(*head) = temp;	
}

FileInfo get_node(FileNode **head) {
	if (head == NULL) {
		exit; 
	}
	FileNode *temp = *head;
	FileInfo info = (*head)->info;
	*head = (*head)->next;
	free(temp);
	return info;
}

int compare_buffers (char *buf1, char *buf2, int max_size) {
	int diff;
	char *temp1 = buf1;
	char *temp2 = buf2;
	int compared_bytes = 0;
	do {
		diff = *temp1++ - *temp2++;
		compared_bytes++;
	} 
	while (compared_bytes < max_size && !diff);

	if (!diff) {
		return compared_bytes;
	} 
	else {
		return -1;
	}
}

void compare_files (FileInfo info1, FileInfo info2, char *exefile) {

	FILE *file1 = fopen(info1.name, "rb");
	FILE *file2 = fopen(info2.name, "rb");
	int compared_bytes = 0;

	if (file1 == NULL) {
		fprintf(stderr, "%d : %s : %s : error opening\n", getpid(), exefile, info1.name);
	} 
	else if (file2 == NULL) {
		fprintf(stderr, "%d : %s : %s : error opening\n", getpid(), exefile, info2.name);
	}
	else {
		char *buf1 = (char*) malloc(sizeof(char) * info1.size);
		char *buf2 = (char*) malloc(sizeof(char) * info2.size);
		fread(buf1, 1, info1.size, file1);
		fread(buf2, 1, info2.size, file2);
		if (buf1 == NULL || buf2 == NULL)
		{
			fclose(file1);
			fclose(file2);
			return;
		}

		/*if (!memcmp(buf1, buf2, info1.size) && info1.size != 0) {
			printf("%d : %s : %s = %s %ld\n", getpid(), exefile, info1.name, info2.name, info1.size);
		}  */

		int compared = compare_buffers(buf1, buf2, info1.size);
		if (compared > 0) {
			printf("%d : %s = %s %d\n", getpid(), info1.name, info2.name, compared);
		}

		free(buf1);
		free(buf2);
		
		if (fclose(file1) != 0 ) {
			fprintf(stderr, "%d : %s : %s : %s\n", getpid(), exefile, info1.name, strerror(errno));		
		}

		if (fclose(file2) != 0 ) {
			fprintf(stderr, "%d : %s : %s : %s\n", getpid(), exefile, info2.name, strerror(errno));		
		}
	}
		
}

void create_process (FileInfo info1, FileInfo info2, char *exefile) {
	int result;

 	while (process_count >= process_limit) {
 		wait(&result);
 		process_count--;
	}

 	if (result == -1) {
 		fprintf(stderr, "%d : %s: %s\n", getpid(), exefile, strerror(errno));
 		exit(0);
 	}

 	if (process_count <= process_limit) {
 		pid_t pid = fork();
 		if ( pid < 0 ) {
 			fprintf(stderr, "%d : %s : %s\n", getpid(), exefile, strerror(errno));
 			return;
 		}
 		else if ( pid == 0 ) {
 			compare_files(info1, info2, exefile);
 			exit(0); 			
 		}
 		else {
 			process_count++;
		}
 	}
}

void get_file_list (char *dir_path, FileNode **list_tail, char *exefile) {
	DIR *dir;
	struct stat info;
	struct dirent *entry;
	FileInfo temp;
	FILE *f;

	if ( !(dir = opendir(dir_path))) {
		fprintf(stderr, "%d : %s : %s : error opening\n",  getpid(), exefile, dir_path);
		return;
	}

	while ( (entry = readdir(dir)) != NULL ) {
		char *path = (char *) malloc(sizeof(char)*(strlen(dir_path)+strlen(entry->d_name) + 2));
		sprintf(path, "%s/%s", dir_path, entry->d_name);

		if (entry->d_type == DT_DIR) {
			if ( strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		        	continue;
		    	get_file_list(path, list_tail, exefile);
		} 
		else if (entry->d_type == DT_REG) {
				f = fopen(path, "r");
				if (f != NULL)
				{
					stat(path, &info);
					fclose(f);
					temp.name = (char*) malloc(sizeof(char) * strlen(path) + 1);
					strcpy(temp.name, path);
					temp.size = info.st_size;
					add_node(list_tail, temp);
				}
				else {
					fprintf(stderr, "%d : %s : %s : error opening file\n",  getpid(), exefile, path);
				}
		}
		free(path);
	}

	if (errno != 0) {
		fprintf(stderr, "%d : %s : %s : %s\n",  getpid(), exefile, dir_path, strerror(errno));		
	}

	if ( closedir(dir) != 0 ) {
		fprintf( stderr, "%d : %s : %s : %s\n", getpid(), exefile, dir_path, strerror(errno));
		return;
	}
}

void view_list (FileNode *head) {
	FileNode *temp = head;
	while (temp != NULL) {
		printf("%d : %s\n", getpid(), temp->info.name);
		temp = temp->next;
	}
}

void compare_lists (FileNode *head1, FileNode *head2, char *exefile) {
	if (!head1 || !head2) return;
	FileNode *temp;
	FileInfo info;
	do {
		info = get_node(&head1);
		temp = head2;
		while (temp != NULL) {
			if (info.size == temp->info.size) {
				create_process(info, temp->info, exefile);
			}	
			temp = temp->next;	
		}	
	} 
	while (head1 != NULL);
}

void free_memory (FileNode *head) {
    FileNode *temp = head;
    while (head != NULL)
    {
       temp = head;
       head = head->next;
       free(temp);
    }

}

int main(int argc, char *argv[]) {

    char *exefile = basename(argv[0]);
	char *path_to_dir1 = (char *)malloc(sizeof(char) * PATH_MAX);
	char *path_to_dir2 = (char *)malloc(sizeof(char) * PATH_MAX);
	process_count = 1;
	
	if (argc != 4) {
		fprintf( stderr, "%d : %s: incorrect number of arguments!\n", getpid(), exefile);
		return 1;
	}

	process_limit = atoi(argv[3]);

	if (process_limit == 0) {
		fprintf( stderr, "%d : %s: incorrect number of processes!\n", getpid(), exefile);
		return 1;
	} 

	if (realpath(argv[1], path_to_dir1) != NULL) {
		if (realpath(argv[2], path_to_dir2) != NULL) {
			FileNode *file_list1 = NULL;
			FileNode *file_list2 = NULL;
		    get_file_list(path_to_dir1, &file_list1, exefile);
			get_file_list(path_to_dir2, &file_list2, exefile);
			compare_lists(file_list1, file_list2, exefile);
			
			while(process_count != 1) {
				wait(NULL);
				process_count--;
			}
			free_memory(file_list2); 
		}
		else {
	        	fprintf(stderr, "%d : %s : %s \n", getpid(), exefile, strerror(errno));
	    } 
	}
	else {
		fprintf(stderr, "%d : %s : %s \n", getpid(), exefile, strerror(errno));
	}

	free(path_to_dir1);
	free(path_to_dir2);

	return 0;
}
