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
	int path_length = strlen(dir_name) + strlen(filename) + 1;
	printf("1\n");
    char path_name[path_length];
	 printf("2\n");
    strcpy(path_name, dir_name);
     printf("3\n");
	strcat(path_name, "/");
	 printf("4\n");
    strcat(path_name, filename);
	printf("path name: %s\n", path_name);
	FILE *fp = fopen(path_name, "w+");
	if(fp == NULL)
	{   
		return NULL;
	}
	return fp;
}

FILE* make_file_inode(char *dir_name, int inode_num) {
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
	FILE *fp = fopen(path_name, "a");
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

int already_found(int found_inodes[], int size, int inode) {
    for (int i = 0; i < size; i++) {
        if (found_inodes[i] == inode) {
            return 1;
        }
    }
    return 0;
}

void find_filenames(char filenames[][EXT2_NAME_LEN + 1], int jpg_inodes[], int num_jpg_inodes, int fd, int found_inodes[]) {
    int inode_num = 1;
    int found_size = 0;
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
                    //	if (already_found(found_inodes, found_size, curr_inode_num)) {
                     //       break;
                     //   }
                        printf("inode: %d offset: %d blocksize: %d group num: %d\n", curr_inode_num,offset, block_size, curr_group);
                        int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
					    if (name_len == 0) {
                            offset += 1;
                            continue;
                        }
                        if (already_found(found_inodes, found_size, curr_inode_num)) {
                             break;
                         }
                    	char name [EXT2_NAME_LEN + 1];
						strncpy(name, dentry->name, name_len);
						name[name_len] = '\0';
						//printf("OFFSET: %d, INODE: %d NAME LEN:%d\n", offset, curr_inode_num, name_len);
						//printf("Entry name is --%s--\n", name);
                        printf("file name length: %d filename: %s\n", name_len, name);
						strcpy(filenames[curr_inode_num - 1], name);
                        found_inodes[found_size++] = curr_inode_num;
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

void print_filenames(char filenames[][EXT2_NAME_LEN + 1], int jpg_inodes[], int size) {
	printf("ALL FILENAMES %d\n", size);
	for (int i = 0; i < size; i++) {
		int inode_num = jpg_inodes[i];
		printf("inode: %d , filename: %s\n", inode_num, filenames[inode_num - 1]);
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

	printf("num groups: %d\n", num_groups);
	unsigned int inode_num = 1;
	int regular_file_count = 0; 
	int jpg_file_count = 0; 
	int total_inodes = num_groups * inodes_per_group;
	char filenames[total_inodes][EXT2_NAME_LEN + 1];
	int jpg_inodes[total_inodes];
    int found_filename_inodes[total_inodes];
	for (int i = 0; i < total_inodes; i++) {
		jpg_inodes[i] = -1;
	}
	int num_jpg_inodes = find_jpg_inodes(jpg_inodes, fd);
	print_jpg_inode_array(jpg_inodes, num_jpg_inodes);
	find_filenames(filenames, jpg_inodes, num_jpg_inodes, fd, found_filename_inodes);
	print_filenames(filenames, jpg_inodes, num_jpg_inodes);
	for (unsigned int curr_group = 0; curr_group < num_groups; curr_group++) {
		printf("curr group: %d\n", curr_group);
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
				printf("found a directory at inode num %d!\n", inode_num);
				lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
				int res = read(fd,&buffer,block_size);
				//int res = snprintf(buffer, block_size, "%s", fd); 
				if (res == -1) {
					exit(1);
				} 
			}
			else if (S_ISREG(inode->i_mode)) {
				regular_file_count++; 
				lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);   
				int res = read(fd,&buffer,block_size);
				if (res == -1) {
					// printf("read error\n");
				} else {
					//   printf("read success\n");
				}
				int is_jpg = 0;
				if (buffer[0] == (char)0xff && buffer[1] == (char)0xd8 && buffer[2] == (char)0xff &&
						(buffer[3] == (char)0xe0 || buffer[3] == (char)0xe1 || buffer[3] == (char)0xe8)) 
				{
					is_jpg = 1;
					jpg_file_count++; 
					printf("is_jpg: %d, inode: %d, group: %d\n", is_jpg, inode_num,curr_group);
					FILE *fp_inode = make_file_inode((char *)argv[2], inode_num);
					if (fp_inode == NULL) {
						exit(1);
					}
					char filename[EXT2_NAME_LEN];
					strcpy(filename, filenames[inode_num -1]);
					FILE *fp_filename = make_file_filename((char *)argv[2], (char *)filename);
					if (fp_filename == NULL) {
						exit(1);
					}

					unsigned int bytes_written = 0;
					unsigned int target_bytes = inode->i_size;
					printf("ext2 n blocks: %u\n", EXT2_N_BLOCKS);
					printf("inode file size: %u\n", inode->i_size);
					for(unsigned int i=0; i<EXT2_N_BLOCKS; i++) {
						if (inode->i_block[i] == 0) {
							printf("exit\n");
							break;
						}


						if(i == 13){
							lseek(fd, BLOCK_OFFSET(curr_group * blocks_per_group + inode->i_block[i]), SEEK_SET); 
							unsigned int double_indirect_block_number[256]; 
							read(fd, &double_indirect_block_number, 256*4); 
							for(int index = 0; index < 256; index++){
								unsigned int data_block_num = double_indirect_block_number[index]; 
								lseek(fd, BLOCK_OFFSET(curr_group * blocks_per_group + data_block_num), SEEK_SET);
								unsigned int ptrs[256];

								read(fd,&ptrs, 256*4);
								for (int index = 0; index < 256; index++) {
									unsigned int data_block_num = ptrs[index];
									lseek(fd, BLOCK_OFFSET(curr_group * blocks_per_group + data_block_num), SEEK_SET);
									int result = read(fd,&buffer, block_size);
									if (result == -1) {
										printf("read error IBLOCK\n");
									}

									unsigned int bytes_remaining = target_bytes - bytes_written;
									int count = block_size / sizeof(char);
									if (bytes_remaining < block_size) {
										count = bytes_remaining / sizeof(char);
									}
									//int results = fputs(buffer, fp);
									int results = fwrite(buffer, sizeof(char), count, fp_inode);
									fwrite(buffer, sizeof(char), count, fp_filename);
									if (results == EOF) {
										printf("error writing result to file\n");
										exit(1);
									}
									bytes_written += results;
					//				printf("bytes written so far: %d\n", bytes_written);

								}	

							}


						}

						else if (i == 12) {
							printf("BLOCK 12!!! inode iblock[12]: %u\n", inode->i_block[i]);
							lseek(fd, BLOCK_OFFSET(curr_group * blocks_per_group + inode->i_block[i]), SEEK_SET);
							unsigned int ptrs[256];
							read(fd, &ptrs, 256*4);
							for (int index = 0; index < 256; index++) {
								unsigned int data_block_num = ptrs[index];
								lseek(fd, BLOCK_OFFSET(curr_group * blocks_per_group + data_block_num), SEEK_SET);
								int result = read(fd,&buffer, block_size);
								if (result == -1) {
									printf("read error IBLOCK\n");
								} 

								unsigned int bytes_remaining = target_bytes - bytes_written;
								int count = block_size / sizeof(char);
								if (bytes_remaining < block_size) {
									count = bytes_remaining / sizeof(char);
								} 
								//int results = fputs(buffer, fp);
								int results = fwrite(buffer, sizeof(char), count, fp_inode);
								fwrite(buffer, sizeof(char), count, fp_filename);
								if (results == EOF) {
									printf("error writing result to file\n");
									exit(1);    
								} 
								bytes_written += results; 
								printf("bytes written so far: %d\n", bytes_written);

							}
						} else {
							//off_t offset = BLOCK_OFFSET(inode->i_block[i]);
							lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);
							printf("iblock[%d] : %u\n", i, inode->i_block[i]);
							int result = read(fd,&buffer, block_size);
							//int result = snprintf(buffer, block_size, "%c", fd);
							if (result == -1) {
								printf("read error IBLOCK\n");
							}

							unsigned int bytes_remaining = target_bytes - bytes_written;
							int count = block_size / sizeof(char);
							if (bytes_remaining < block_size) {
								count = bytes_remaining / sizeof(char);
							}
							//int results = fputs(buffer, fp);
							int results = fwrite(buffer, sizeof(char), count, fp_inode);
							fwrite(buffer, sizeof(char), count, fp_filename);
							if (results == EOF) {
								printf("error writing result to file\n");
								exit(1);
							}
							bytes_written += results;
							printf("bytes written so far: %d\n", bytes_written);

						}
					}
					//printf("bytes written: %d\n", bytes_written);
					printf("bytes that should be written: %d\n", inode->i_size);
					fclose(fp_inode);
					fclose(fp_filename);
				}
			}
			else {
				// this inode represents other file types
				//printf("this inode is another type\n");
			}
			inode_num++;
		}
		//}
} 

printf("regular file count: %d\n", regular_file_count); 
printf("jpg file count: %d\n", jpg_file_count);



//    printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);
//iterate the first inode blocki
/*    off_t start_inode_table = locate_inode_table(0, &group);
      for (unsigned int i = 0; i < inodes_per_block; i++) {
      printf("inode %u: \n", i);
      struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
      read_inode(fd, 0, start_inode_table, i, inode); */
/* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
 * or once simplified, i_blocks/(2<<s_log_block_size)
 * https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
 */
/*     unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
       printf("number of blocks %u\n", i_blocks);
       printf("Is directory? %s \n Is Regular file? %s\n",
       S_ISDIR(inode->i_mode) ? "true" : "false",
       S_ISREG(inode->i_mode) ? "true" : "false");
       */
// print i_block numberss
/*       for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
	 {       if (i < EXT2_NDIR_BLOCKS)  */                               /* direct blocks */
//        printf("Block %2u : %u\n", i, inode->i_block[i]);
//        else if (i == EXT2_IND_BLOCK)                             /* single indirect block */
//            printf("Single   : %u\n", inode->i_block[i]);
//        else if (i == EXT2_DIND_BLOCK)                            /* double indirect block */
//            printf("Double   : %u\n", inode->i_block[i]);
//        else if (i == EXT2_TIND_BLOCK)                            /* triple indirect block */
//            printf("Triple   : %u\n", inode->i_block[i]);

//  }


// }	
close(fd);
}
