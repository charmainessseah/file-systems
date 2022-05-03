#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

FILE* make_file_filename(char *dir_name, char *filename) {
    printf("dir name: %s len: %ld\n", dir_name, strlen(dir_name));
    printf("filename: %s len: %ld\n", filename, strlen(filename));
    char path_length = strlen(dir_name) + strlen(filename) + 1;
    char path_name[path_length];
    strcpy(path_name, dir_name);
    strcat(path_name, "/");
    strcat(path_name, filename);
    printf("path name: %s\n", path_name);
    FILE *fp = fopen(path_name, "w+");
    if(fp == NULL)
    {   
        return NULL;
    }
    return fp;
}

FILE* make_file_inode_num(char *dir_name, int inode_num) {
    int num_digits = 0;
    int n = inode_num;
    do {
        n /= 10;
        num_digits++;
    } while (n != 0);

    char inode_str[num_digits];
    sprintf(inode_str, "%d", inode_num);
    int path_length = strlen(dir_name) + num_digits + 10;

    char path_name[path_length];
    strcpy(path_name, dir_name);
    strcat(path_name, "/file-");
    strcat(path_name, inode_str);
    strcat(path_name, ".jpg");  
    FILE *fp = fopen(path_name, "w+");
    if(fp == NULL)
    {
        return NULL;
    }
    return fp;
}

int find_jpg_inodes(int jpg_inodes[], int fd) { 
    int num_jpg_inodes = 0;
    int inode_num = 1;
    for (unsigned int curr_group = 0; curr_group < num_groups; curr_group++) {
        struct ext2_super_block super;
        struct ext2_group_desc group;
        read_super_block(fd, curr_group, &super);
        read_group_desc(fd, curr_group, &group);
        off_t inode_table = locate_inode_table(curr_group, &group);
        for (unsigned int inode_num_group = 0; inode_num_group < inodes_per_group; inode_num_group++) {
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(fd, curr_group, inode_table, inode_num, inode);
            char buffer[block_size];
            if (S_ISREG(inode->i_mode)) {
                lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);   
                int res = read(fd,&buffer,block_size);
                if (res == -1) {
                    printf("error: find_jpg_inodes reading into buffer\n");
                    exit(1);
                } 
                if (buffer[0] == (char)0xff && buffer[1] == (char)0xd8 && buffer[2] == (char)0xff &&
                        (buffer[3] == (char)0xe0 || buffer[3] == (char)0xe1 || buffer[3] == (char)0xe8)) 
                {
                        jpg_inodes[num_jpg_inodes++] = inode_num;
                }    
            }
            inode_num++;
        }
    }   
    return num_jpg_inodes; 
}

int is_target_inode(int jpg_inodes[], int num_jpg_inodes, int target_inode) {
    for (int i = 0; i < num_jpg_inodes; i++) {
        if (jpg_inodes[i] == target_inode) {
            return 1;
        }
    }
    return 0;
}

void find_filenames(char filenames[][EXT2_NAME_LEN], int jpg_inodes[], int num_jpg_inodes, int fd) {
    int inode_num = 0;
    for (unsigned int curr_group = 0; curr_group < num_groups; curr_group++) {
        struct ext2_super_block super;
        struct ext2_group_desc group;
        read_super_block(fd, curr_group, &super);
        read_group_desc(fd, curr_group, &group);
        off_t inode_table = locate_inode_table(curr_group, &group);
        for (unsigned int inode_num_group = 0; inode_num_group < inodes_per_group; inode_num_group++) {
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(fd, curr_group, inode_table, inode_num, inode);
            char buffer[block_size];
            if (S_ISDIR(inode->i_mode)) {
                lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
                int res = read(fd,&buffer,block_size);
                //int res = snprintf(buffer, block_size, "%s", fd); 
                if (res == -1) {
                    exit(1);
                }
                unsigned int offset = 0;
                while(offset <= block_size) {
                    struct ext2_dir_entry *dentry = (struct ext2_dir_entry*) & ( buffer[offset] );
                    int curr_inode_num = dentry->inode;
                    if (is_target_inode(jpg_inodes, num_jpg_inodes, curr_inode_num)) {
                        int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
                        char name [EXT2_NAME_LEN];
                        strncpy(name, dentry->name, name_len);
                        name[name_len] = '\0';
                        //printf("OFFSET: %d, INODE: %d NAME LEN:%d\n", offset, curr_inode_num, name_len);
                        //printf("Entry name is --%s--\n", name);
                        strcpy(filenames[curr_inode_num - 1], name);
                    } 
                    offset += 1;
                }
            }
            inode_num++;
        }
    }
}

void print_jpg_inode_array(int jpg_inodes[], int size) {
    printf("jpg inodes: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", jpg_inodes[i]);
    }
    printf("\n");
}

void print_filenames(char filenames[][EXT2_NAME_LEN], int jpg_inodes[], int size) {
    printf("ALL FILENAMES: ");
    for (int i = 0; i < size; i++) {
        int inode_num = jpg_inodes[i];
        printf("%s ", filenames[inode_num - 1]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(1);
	}

	DIR *dp = opendir(argv[2]);
    if (dp != NULL) {
        printf("error: directory already exists\n");
        exit(1);
    } 

    int mkdir_result = mkdir(argv[2], 0777);
    if (mkdir_result == -1) {
        printf("error: failed to make new directory\n");
        exit(1);
    } 

	int fd;

	fd = open(argv[1], O_RDONLY);    /* open disk image */

    ext2_read_init(fd);

    int total_inodes = num_groups * inodes_per_group;
    char filenames[total_inodes][EXT2_NAME_LEN];
    int jpg_inodes[total_inodes];
    for (int i = 0; i < total_inodes; i++) {
        jpg_inodes[i] = -1;
    }
    int num_jpg_inodes = find_jpg_inodes(jpg_inodes, fd);
    print_jpg_inode_array(jpg_inodes, num_jpg_inodes);
    find_filenames(filenames, jpg_inodes, num_jpg_inodes, fd);
    print_filenames(filenames, jpg_inodes, num_jpg_inodes);

    for (int index = 0; index < num_jpg_inodes; index++) {
        int inode_num = jpg_inodes[index];
        char filename[EXT2_NAME_LEN];
        strcpy(filename, filenames[inode_num - 1]);
        FILE *fp_inode = make_file_inode_num((char *)argv[2], inode_num);
        if (fp_inode == NULL) {
            exit(1);
        }
        FILE *fp_filename = make_file_filename((char *)argv[2], (char *)filename);
        if (fp_filename == NULL) {
            printf("fp_filename is null\n");
            exit(1);
        }
    }
    close(fd);
}

