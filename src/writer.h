#ifndef M2G_WRITER
#define M2G_WRITER

#include <bits/stdc++.h>
#include "structures.h"
#include "reader.h"

bool delete_file(FILE *device, unsigned int inode_destiny, unsigned int inode_content, std::string name_source, directory_entry de_at, inode inode_at);

unsigned int inode_alloc(FILE *device){
    superblock sb;
    sb = read_superblock(device);

    unsigned int block_size = (1024 << sb.pot_block_size);
    unsigned int local_time = std::time(nullptr);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes+ 1;
    unsigned int _indir = sb.num_inodes + 1;
    unsigned int free_position = empty_inode(device);
    unsigned int ad_new_inode = (sb.ad_inode_tab * block_size) + (free_position * 62);
    sb.num_free_inodes = sb.num_free_inodes - 1;

    fseek(device, 0, SEEK_SET);
    fwrite(&sb, sizeof(superblock), 1, device);

    inode new_inode;
    new_inode.type[0] = 1 ;
    new_inode.type[1] = 0 ;
    new_inode.size = 0;
    new_inode.last_access_time = local_time;
    new_inode.creation_time = local_time;
    new_inode.modified_time = local_time;
    new_inode.link_count = 0;
    for(int i = 0; i < 8; i++)
        new_inode.direct_pointers[i] = null_ptr_dir;
    new_inode.inderect_pointer = null_ptr_indir;

    fseek(device, ad_new_inode, SEEK_SET);
    fwrite(&new_inode, sizeof(inode), 1, device);

    dec_free_inodes(device);

    return free_position;
}

bool inode_update(FILE *device, unsigned int inode_index, inode updated_inode){
    superblock sb;
    sb = read_superblock(device);

    unsigned int local_time = std::time(nullptr);
    unsigned int block_size = (1024 << sb.pot_block_size);
    unsigned int ad_updated_inode = (sb.ad_inode_tab * block_size) + (inode_index * 62);

    updated_inode.last_access_time = local_time;
    updated_inode.modified_time = local_time;

    fseek(device, ad_updated_inode, SEEK_SET);

    return fwrite(&updated_inode, sizeof(inode), 1, device);
}

bool init_root_dir(FILE *device, unsigned int inode_index){
    superblock sb;
    sb = read_superblock(device);

    unsigned int block_size = (1024 << sb.pot_block_size);
    unsigned int ad_inode = (sb.ad_inode_tab * block_size) + (inode_index * 62);
    inode updated_inode;

    unsigned int new_block = get_empty_block(device);
    set_block_used(device, new_block);
    updated_inode.direct_pointers[0] = new_block;

    inode_update(device, 0, updated_inode);
    return true;
}

unsigned long write_data_in_inode_from_file(FILE *device, FILE *data, unsigned int inode_index, unsigned long *data_length){
    superblock sb;
    sb = read_superblock(device);

    unsigned int block_size = (1024 << sb.pot_block_size);
    unsigned int ad_inode = (sb.ad_inode_tab * block_size) + (inode_index * 62);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes + 1;
    unsigned long file_size = *data_length;
    inode updated_inode;

    fseek(device, ad_inode, SEEK_SET);
    fread(&updated_inode, sizeof(inode), 1, device);


    while(*data_length > 0){

        if(are_free_blocks(device)){
            unsigned int block = get_empty_block(device);
            unsigned long ad_block = block * block_size;
            bool direct = false;

            set_block_used(device, block);
            fseek(device, ad_block, SEEK_SET);

            /////Gravação dos dados
            if(*data_length >= block_size){
                unsigned char buffer[block_size];
                fread(&buffer, block_size, 1, data);
                fwrite(&buffer, block_size, 1, device);
                *data_length -= block_size;
                updated_inode.size += block_size;
            }
            else{
                unsigned char buffer[*data_length];
                fread(&buffer, *data_length, 1, data);
                fwrite(&buffer, *data_length, 1, device);
                *data_length = 0;
                updated_inode.size += *data_length;
            }
            /////Gravação dos dados

            for(unsigned int j = 0; j < 8; j++){
                if(updated_inode.direct_pointers[j] == null_ptr_dir){
                    updated_inode.direct_pointers[j] = block;
                    direct = true;
                    break;
                }
            }
            if(!direct){ // todos os ponteiros diretos foram usados
                if(updated_inode.inderect_pointer == null_ptr_indir) // o ponteiro para o inode indireto não estava definido
                    updated_inode.inderect_pointer = inode_alloc(device);
                updated_inode.size = file_size;
                write_data_in_inode_from_file(device, data, updated_inode.inderect_pointer, data_length);
            }
        }else{
            updated_inode.size = file_size;
            inode_update(device, inode_index, updated_inode);
            return 0;
        }
    }
    updated_inode.size = file_size;
    inode_update(device, inode_index, updated_inode);
    return updated_inode.size;
}

bool write_in_file_from_inode(FILE *device, FILE *data, unsigned int inode_index, unsigned long *data_length){
    superblock sb;
    sb = read_superblock(device);

    unsigned int block_size = (1024 << sb.pot_block_size);
    unsigned int ad_inode = (sb.ad_inode_tab * block_size) + (inode_index * 62);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes + 1;
    inode updated_inode;

    fseek(device, ad_inode, SEEK_SET);
    fread(&updated_inode, sizeof(inode), 1, device);

    for(unsigned int j = 0; j < 8; j++){
        if(updated_inode.direct_pointers[j] != null_ptr_dir){
            if(*data_length > block_size){
                fseek(device, (updated_inode.direct_pointers[j] * block_size), SEEK_SET);
                unsigned char buffer[block_size];
                fread(&buffer, block_size, 1, device);
                fwrite(&buffer, block_size, 1, data);
                *data_length -= block_size;
            }
            else{
                fseek(device, (updated_inode.direct_pointers[j] * block_size), SEEK_SET);
                unsigned char buffer[*data_length];
                fread(&buffer, *data_length, 1, device);
                fwrite(&buffer, *data_length, 1, data);
                *data_length = 0;
            }
        }
        else{
            return true;
        }
    }
    if(updated_inode.inderect_pointer != null_ptr_indir){
        write_in_file_from_inode(device, data, updated_inode.inderect_pointer, data_length);
    }
    return true;
}

bool write_directory_entry_in_inode(FILE *device, directory_entry de, unsigned int inode_index){
    superblock sb;
    sb = read_superblock(device);

    unsigned int block_size = (1024 << sb.pot_block_size);
    unsigned int ad_inode = (sb.ad_inode_tab * block_size) + (inode_index * 62);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes + 1;
    inode updated_inode;

    char free_dir_name[27];
    memset(free_dir_name, 0, 27);

    fseek(device, ad_inode, SEEK_SET);
    fread(&updated_inode, sizeof(inode), 1, device);

    bool end_dir_pointers = false;

    for(int i = 0; i < 8; i++){
        if(updated_inode.direct_pointers[i] != null_ptr_dir){
            unsigned long pointer = (updated_inode.direct_pointers[i]) * block_size;
            fseek(device, pointer, SEEK_SET);
            for( int j = 0; j < block_size; j += sizeof(directory_entry) ){
                directory_entry temp_de;
                fread(&temp_de, sizeof(directory_entry), 1, device);
                if(temp_de._type == 0 && (!strcmp(temp_de.name, free_dir_name))){
                    fseek(device, -1 * (sizeof(directory_entry)), SEEK_CUR);
                    fwrite(&de, sizeof(directory_entry), 1, device);
                    return true;
                }
            }
        }else{
            unsigned int block = get_empty_block(device);
            unsigned long ad_block = block * block_size;
            set_block_used(device, block);

            updated_inode.direct_pointers[i] = block;
            inode_update(device, inode_index, updated_inode);
            fseek(device, ad_block, SEEK_SET);
            fwrite(&de, sizeof(directory_entry), 1, device);
            return true;
        }
    }

    if(updated_inode.inderect_pointer != null_ptr_indir){
        return write_directory_entry_in_inode(device, de, updated_inode.inderect_pointer);
    }

    unsigned int index_inode = inode_alloc(device);
    updated_inode.inderect_pointer = index_inode;
    inode_update(device, inode_index, updated_inode);
    return write_directory_entry_in_inode(device, de, updated_inode.inderect_pointer);

}

bool remove_dir_entry(FILE *device, char *path){
    superblock sb;
    sb = read_superblock(device);

    std::string destiny_path = path;
    if(destiny_path == "")
        return 0;

    std::vector<std::string> directories_path;
    std::string delimiter = "/";

    size_t pos = 0;
    std::string token;

    while ((pos = destiny_path.find(delimiter)) != std::string::npos) {
        token = destiny_path.substr(0, pos);
        directories_path.push_back(token);
        destiny_path.erase(0, pos + delimiter.length());
    }
    directories_path.push_back(destiny_path);

    std::string at_dir = directories_path[0];
    unsigned int inode_index = find_dir_entry(device, at_dir, 0);

    for(int i = 1; i < directories_path.size(); i++){
        at_dir = directories_path[i];
        inode_index = find_dir_entry(device, at_dir, inode_index);
        if(inode_index == UINT_MAX)
            break;
    }
    return inode_index;
}

directory_entry create_dir_entry(FILE *device, unsigned char type, const char *name){
    directory_entry de;

    unsigned int index_inode = inode_alloc(device);

    de.index_inode = index_inode;
    de._type = type;
    strcpy(de.name, name);

    return de;
}

directory_entry create_dir_entry(FILE *device, unsigned char type, const char *name, unsigned int index_inode){
    directory_entry de;

    de.index_inode = index_inode;
    de._type = type;
    strcpy(de.name, name);

    return de;
}

bool remove_dir_entrie_from_inode(FILE *device, unsigned int index_inode, std::string dir_entrie_name){
    superblock sb;
    sb = read_superblock(device);

    std::vector<std::pair<unsigned int, std::string> > dir_entries;

    unsigned int block_size = (1024 << sb.pot_block_size);
    inode inode_at = get_inode_by_index(device, index_inode);
    unsigned int npointers = ceil(inode_at.size / block_size);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes + 1;

    char free_dir_name[27];
    memset(free_dir_name, 0, 27);

    for(int i = 0; i < 8; i++){
        if(inode_at.direct_pointers[i] != null_ptr_dir){
            unsigned long pointer = (inode_at.direct_pointers[i]) * block_size;
            fseek(device, pointer, SEEK_SET);
            for( int j = 0; j < block_size; j += sizeof(directory_entry) ){
                directory_entry temp_de;
                fread(&temp_de, sizeof(directory_entry), 1, device);
                if(temp_de._type > 0 && temp_de._type < 5){
                    std::string name = temp_de.name;
                    if(name == dir_entrie_name){
                        fseek(device, -sizeof(directory_entry) , SEEK_CUR);
                        int count = sizeof(directory_entry);
                        while(count--)
                            fputc(0, device);
                        return true;
                    }
                }
            }
        }else{
            return false;
        }
    }
    if(inode_at.inderect_pointer != null_ptr_indir){
        return remove_dir_entrie_from_inode(device, inode_at.inderect_pointer, dir_entrie_name);
    }
    return false;
}

void clear_inode (FILE *device, unsigned int index_inode){
    superblock sb;
    sb = read_superblock(device);

    unsigned int block_size = (1024 << sb.pot_block_size);
    inode inode_at = get_inode_by_index(device, index_inode);
    unsigned int npointers = ceil(inode_at.size / block_size);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes + 1;

    if(inode_at.type[0] == 0){
        return;
    }

    for(int i = 0; i < 8; i++){
        if(inode_at.direct_pointers[i] != null_ptr_dir){
            fseek(device, inode_at.direct_pointers[i]*block_size, SEEK_SET);
            int count = block_size;
            while(count --)
                fputc(0, device);
            set_block_free(device, inode_at.direct_pointers[i]);
        }else{
            return;
        }
    }
    if(inode_at.inderect_pointer != null_ptr_indir){
        clear_inode(device, inode_at.inderect_pointer);
    }
    inode free_inode;
    free_inode.type[0] = 0;
    inode_update(device, index_inode, free_inode);
    inc_free_inodes(device);
}

bool clear_directory(FILE *device, unsigned int index_inode){
    superblock sb;
    sb = read_superblock(device);

    bool result = true;

    unsigned int block_size = (1024 << sb.pot_block_size);
    inode inode_at = get_inode_by_index(device, index_inode);
    unsigned int npointers = ceil(inode_at.size / block_size);
    unsigned int null_ptr_dir = sb.num_blocks + 1;
    unsigned int null_ptr_indir = sb.num_inodes + 1;

    for(int i = 0; i < 8; i++){
        if(inode_at.direct_pointers[i] != null_ptr_dir){
            fseek(device, inode_at.direct_pointers[i]*block_size, SEEK_SET);
            for( int j = 0; j < block_size; j += sizeof(directory_entry) ){
                directory_entry temp_de;
                fread(&temp_de, sizeof(directory_entry), 1, device);
                if(temp_de._type != 0){
                    if(!strcmp(temp_de.name, "..")){
                        result = result && remove_dir_entrie_from_inode(device, index_inode, temp_de.name);
                        continue;
                    }
                    inode inode_dir_entry = get_inode_by_index(device, temp_de.index_inode);
                    result = result && delete_file(device, index_inode, temp_de.index_inode, temp_de.name, temp_de, inode_dir_entry);
                }
            }
        }else{
            return result;
        }
    }
    if(inode_at.inderect_pointer != null_ptr_indir){
        result = result && clear_directory(device, inode_at.inderect_pointer);
    }
    return result;
}

bool delete_file(FILE *device, unsigned int inode_destiny, unsigned int inode_content, std::string name_source, directory_entry de_at, inode inode_at){
        
    unsigned char type_at = de_at._type;

    bool result = false;
    if(inode_at.link_count == 0){
        if(type_at == 1){//arquivo
            result = remove_dir_entrie_from_inode(device, inode_destiny, name_source);
            if (result){
                clear_inode(device, inode_content);
                result = true;
            }
        }
        else if (type_at == 2){//diretorio
            result = clear_directory(device, inode_content);
            result = result && remove_dir_entrie_from_inode(device, inode_destiny, name_source);
        }else if(type_at == 3){//link
            result = remove_dir_entrie_from_inode(device, inode_destiny, name_source);
        }else if(type_at == 4){//hardlink
            result = remove_dir_entrie_from_inode(device, inode_destiny, name_source);
            if(result){
                clear_inode(device, inode_content);
                result =  true;
            }
        }
    }else{
        result = remove_dir_entrie_from_inode(device, inode_destiny, name_source);
        if(result && type_at != 3){
            inode_at.link_count -= 1;
            inode_update(device, inode_content, inode_at);
        }
    }
    return result;
}

#endif
