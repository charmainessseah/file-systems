#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>


FILE* make_file(char *dir_name, int inode_num) {
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
    for (unsigned int curr_group = 0; curr_group < num_groups; curr_group++) {
        printf("curr group: %d\n", curr_group);
        struct ext2_super_block super;
        struct ext2_group_desc group;
        read_super_block(fd, curr_group, &super);
        read_group_desc(fd, curr_group, &group);      
        off_t inode_table = locate_inode_table(curr_group, &group);
        // we should get file names first
        for (unsigned int inode_num_group = 0; inode_num_group < inodes_per_group; inode_num_group++) {
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(fd, curr_group, inode_table, inode_num, inode);
            //unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
/*            printf("number of blocks %u\n", i_blocks);
            printf("Is directory? %s \n Is Regular file? %s\n",
                    S_ISDIR(inode->i_mode) ? "true" : "false",
                    S_ISREG(inode->i_mode) ? "true" : "false");

            // print i_block numberss
            for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
            {       if (i < EXT2_NDIR_BLOCKS)                                
                printf("Block %2u : %u\n", i, inode->i_block[i]);
                else if (i == EXT2_IND_BLOCK)                             
                    printf("Single   : %u\n", inode->i_block[i]);
                else if (i == EXT2_DIND_BLOCK)                            
                    printf("Double   : %u\n", inode->i_block[i]);
                else if (i == EXT2_TIND_BLOCK)                           
                    printf("Triple   : %u\n", inode->i_block[i]);

            } */

            char buffer[block_size];
            if (S_ISDIR(inode->i_mode)) {
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
                    FILE *fp = make_file((char *)argv[2], inode_num);
                    if (fp == NULL) {
                        exit(1);
                    }
                    for(unsigned int i=0; i<EXT2_N_BLOCKS; i++) {
                        if (inode->i_block[i] == 0) {
                            break;
                        }
                        lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET);
                        printf("iblock[%d] : %u\n", i, inode->i_block[i]);
                        int result = read(fd,&buffer,block_size);
                        if (result == -1) {
                            printf("read error IBLOCK\n");
                        }
                        //int results = fputs(buffer, fp);
                        int results = fwrite(buffer, sizeof(char), inode->i_size, fp);
                        if (results == EOF) {
                            printf("error writing result to file\n");
                            exit(1);
                        }

                    }
                    fclose(fp);
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
