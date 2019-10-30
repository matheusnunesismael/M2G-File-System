
#ifndef M2G_UTILS
#define M2G_UTILS

#define POINTERS_PER_INODE 8

#include <stdio.h>
#include <stdlib.h>
#include "reader.h"
#include "bitmask.h"

unsigned long get_size (FILE *stream) {
    unsigned long size;
    fseek(stream, 0L, SEEK_END);
    size = (unsigned long) ftell(stream);
    rewind(stream);
    return size;
}

bool are_free_blocks(FILE *device){
    superblock sb;
    sb = read_superblock(device);
    if(sb.num_free_blocks > 0)
        return true;
    return false;
}

bool are_free_inodes(FILE *device){
    superblock sb;
    sb = read_superblock(device);
    if(sb.num_free_inodes > 0)
        return true;
    return false;
}

unsigned int num_blocks(FILE *device){
    superblock sb;
    sb = read_superblock(device);
    return sb.num_blocks;
}

unsigned int num_inodes(FILE *device){
    superblock sb;
    sb = read_superblock(device);
    return sb.num_inodes;
}

unsigned int get_empty_block(FILE *device){
    superblock sb;
    sb = read_superblock(device);
    unsigned int ad_block_bitmp = sb.ad_block_bitmp;
    unsigned int num_blocks = sb.num_blocks;
    unsigned int index_free_block = 49;

    unsigned char bitmap[num_blocks];

    fseek(device, ad_block_bitmp, SEEK_SET);
    fread(bitmap, sizeof(char) * num_blocks, 1, device);

    for(unsigned int i = 0; i < num_blocks; i++){
        if(!return_bit_at(bitmap, i)){
            index_free_block =  i;
            break;
        }
    }
    return index_free_block;
}

bool set_block_used(FILE *device, unsigned int block){
    superblock sb;
    sb = read_superblock(device);
    unsigned int ad_block_bitmp = sb.ad_block_bitmp;
    unsigned int num_blocks = sb.num_blocks;

    unsigned char bitmap[num_blocks];

    std::cout << "passou aqui :"<< block << std::endl;

    fseek(device, ad_block_bitmp, SEEK_SET);
    fread(bitmap, sizeof(unsigned char) * num_blocks, 1, device);

    toggle_bit_at(bitmap, block, true);

    fseek(device, ad_block_bitmp, SEEK_SET);
    fwrite(bitmap, sizeof(unsigned char) * num_blocks, 1, device);
    
    return true;
}

unsigned int empty_inode(FILE *device){
    superblock sb;
    sb = read_superblock(device);
    unsigned int num_inodes = sb.num_inodes;
    unsigned int ad_inode_tab = sb.ad_inode_tab;
    unsigned int index_inode_free = 49;
    inode inode_table[num_inodes];

    fseek(device, ad_inode_tab, SEEK_SET);
    fread(inode_table, sizeof(inode) * num_inodes, 1, device);

    for(unsigned int i = 0; i < num_inodes; i++){
        inode inode_at = inode_table[i];
        unsigned char type_inode_at = inode_at.type[0];
        if(type_inode_at == 0){
            index_inode_free = i;
            break;
        }    
    }
    return index_inode_free;
}

#endif