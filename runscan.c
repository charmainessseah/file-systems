#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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

/*    struct ext2_super_block super;
    struct ext2_group_desc group;

    // example read first the super-block and group-descriptor
    read_super_block(fd, 0, &super);
    read_group_desc(fd, 0, &group); */
    printf("HELLO WORLD!!!\n");
    printf("num groups: %d\n", num_groups);
    unsigned int inode_num = 1;
    for (unsigned int curr_group = 0; curr_group < num_groups; curr_group++) {
        struct ext2_super_block super;
        struct ext2_group_desc group;
        read_super_block(fd, curr_group, &super);
        read_group_desc(fd, curr_group, &group);      
        off_t inode_table = locate_inode_table(curr_group, &group);
        for (unsigned int inode_num_group = 0; inode_num_group < inodes_per_block; inode_num_group++) {
            printf("INODE INFO!!\n");
            printf("inode %u: \n", inode_num);
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(fd, curr_group, inode_table, inode_num, inode);
            unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
            printf("inode size: %u\n", inode->i_size);
            printf("inode link count: %u\n", inode->i_links_count);
            printf("number of blocks: %u\n", i_blocks);
            if (S_ISDIR(inode->i_mode)) {
                // this inode represents a directory
                printf("inode is a directory\n");
                inode->i_block[0]->inode
                char buffer[block_size];
                lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
                int res = read(fd,&buffer,block_size);
                if (res == -1) {
                    printf("read error\n");
                } else {
                    printf("read success\n");
                }

                dentry = (struct ext2_dir_entry*) & ( buffer[68] );

                int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly

                char name [EXT2_NAME_LEN];
                strncpy(name, dentry->name, name_len);
                name[name_len] = '\0';

                printf("Entry name is --%s--", name);   


            }
            else if (S_ISREG(inode->i_mode)) {
                printf("inode is a file\n");
                // this inode represents a regular file
                char buffer[block_size];
                buffer[0] = 'h';
                buffer[1] = 'i';
                buffer[2] = '\0';
                printf("buffer before: %s\n", buffer);

                lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);   
                int res = read(fd,&buffer,block_size);
                if (res == -1) {
                    printf("read error\n");
                } else {
                    printf("read success\n");
                }
                printf("buffer after: %s\n", buffer);            
                int is_jpg = 0;
                if (buffer[0] == (char)0xff && buffer[1] == (char)0xd8 && buffer[2] == (char)0xff &&
                        (buffer[3] == (char)0xe0 || buffer[3] == (char)0xe1 || buffer[3] == (char)0xe8)) 
                {
                    is_jpg = 1;
                }
                printf("is_jpg: %d\n", is_jpg);
                // if it is a jpg then we should copy the contens of the file to an output file, using the inode number as the file name eg, 'output/file-18.jpg'
            }
            else {
                // this inode represents other file types
                printf("this inode is another type\n");
            }
            inode_num++;
        }
    } 



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
