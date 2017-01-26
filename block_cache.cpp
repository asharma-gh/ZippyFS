#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <cerrno>
#include <algorithm>
using namespace std;

BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {}

int
BlockCache::remove(string path) {
    if (meta_data_.find(path) == meta_data_.end())
        return -1;
    for (auto entry : meta_data_) {
        auto vec = entry.second->get_refs();
        if (find(vec.begin(), vec.end(), path) != vec.end())
            entry.second->dec_link(path);
    }
    meta_data_.erase(path);
    size_--;
    return 0;
}

int
BlockCache::make_file(string path, mode_t mode) {
    shared_ptr<Inode> ptr(new Inode(path));
    ptr->set_mode(mode);
    meta_data_[path] = ptr;
    size_++;
    return 0;
}

int
BlockCache::load_from_shdw(string path) {
    cout << "loading to cache from shdw " << path << endl;
    int res = load_to_shdw(path.c_str());
    // construct path to shdw
    string shdw_file_path = path_to_shdw_ + path.substr(1);
    cout << "path to shdw file " << shdw_file_path << endl;

    if (res == -1) {
        cout << "could not find the thing " << endl;
        return -1;
    }
    cout << " thing is in shdw" << endl;
    struct stat st;
    stat(shdw_file_path.c_str(), &st);

    if (S_ISDIR(st.st_mode)) {
        cout << "made dir" << endl;
        make_file(path, st.st_mode);
        return 0;
    }
    // open, extract bytes
    FILE* file;
    if ((file = fopen(shdw_file_path.c_str(), "r")) == NULL) {
        return -1;
    }
    // get file size
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char contents[fsize];
    memset(contents, 0, sizeof(contents) / sizeof(char));
    fread(contents, fsize, 1, file);
    fclose(file);
    // add this file to cache
    make_file(path, st.st_mode);
    write(path, (uint8_t*)contents, fsize, 0);
    cout << "finished loading to shdw" << endl;
    return 0;
}

int
BlockCache::getattr(string path, struct stat* st) {
    if (in_cache(path) == -1 && load_from_shdw(path) == -1)
        return -ENOENT;
    else
        return meta_data_[path]->stat(st);
}

vector<BlockCache::index_entry>
BlockCache::readdir(string path) {
    vector<BlockCache::index_entry> ents;
    for (auto entry : meta_data_) {
        if (entry.second->get_link() == 0)
            continue;
        char* dirpath = strdup(entry.first.c_str());
        dirpath = dirname(dirpath);
        if (strcmp(dirpath, path.c_str()) == 0) {
            struct stat st;
            entry.second->stat(&st);
            index_entry ent;
            ent.path = entry.first;
            ent.deleted = 0;
            ent.added_time = entry.second->get_ull_mtime();
            ents.push_back(ent);
        }
        free(dirpath);
    }

    return ents;
}

int
BlockCache::write(string path, const uint8_t* buf, size_t size, size_t offset) {
    if (in_cache(path) == -1) {
        cout << "loading from shdw " << path << endl;
        if (load_from_shdw(path) == -1) {
            cout << "error loading file in " << endl;
            return -1;
        }

    }
    // create blocks for buf
    uint64_t num_blocks = Util::ulong_ceil(size + offset, Block::get_logical_size());
    /*** create inode for write if needed ***/
    shared_ptr<Inode> inode;
    inode = meta_data_.find(path)->second;
    if (inode->get_size() < size + offset)
        inode->set_size(size + offset);

    // for this file, make a block and add it to cache
    uint64_t curr_idx = 0;
    uint64_t block_size = 0;
    bool offsetted = false;
    for (unsigned int block_idx = offset / Block::get_logical_size();  block_idx < num_blocks; block_idx++) {

        curr_idx += block_size;

        if (size < curr_idx + Block::get_logical_size())
            block_size = size - curr_idx;
        else
            block_size = Block::get_logical_size();

        int offset_amt = 0;
        if (offsetted == false) {
            offset_amt = offset < Block::get_logical_size() ? offset :
                         (offset % Block::get_logical_size());
            offsetted = true;
        }
        cout << "block idx " << block_idx << endl;
        if (inode->has_block(block_idx)) {
            cout << "WE HAVE THE BLOCK" << endl;
            shared_ptr<Block> temp = inode->get_block(block_idx);
            temp->insert(buf, block_size, offset_amt);
            cout << "overrided block" << endl;


        } else {
            shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));
            // add newly formed block to the inode
            inode->add_block(block_idx, ptr);
        }
    }
    assert(curr_idx + block_size == size);

    // record meta data to cache_data
    // get prev inode if it exists
    meta_data_[path] = inode;
    return size;
}

int
BlockCache::read(string path, uint8_t* buf, uint64_t size, uint64_t offset) {
    if (in_cache(path) == -1) {
        cout << " READ: loading from shdw " << path << endl;
        load_from_shdw(path);
    }
    return meta_data_[path]->read(buf, size, offset);
}

int
BlockCache::truncate(string path, uint64_t size) {
    if (meta_data_.find(path) == meta_data_.end())
        return -1;
    shared_ptr<Inode> ptr = meta_data_[path];
    ptr->set_size(size);
    return 0;
}

int
BlockCache::in_cache(string path) {
    (void)path;
    return meta_data_.find(path) != meta_data_.end() || path.compare("/") == 0 ? 0 : -1;
}

int
BlockCache::flush_to_shdw(int on_close) {

    cout << "SIZE " << size_ << endl;
    if (size_ < MAX_SIZE && on_close == 0)
        return -1;
    // make index file for cache
    string idx_path = path_to_shdw_ + "index.idx";
    cout << "PATH TO SHDW " << idx_path << endl;

    // create files for each item in cache
    for (auto const& entry : meta_data_) {
        // load previous version to shadow director
        cout  << "NAME " << entry.first << endl;

        // load file or parent to shdw
        int res = load_to_shdw(entry.first.c_str());
        cout << "this was reached " << endl;
        if (res == -1) {
            char* dirpath = strdup(entry.first.c_str());
            cout << "dirp " << dirpath << endl;
            dirpath = dirname(dirpath);
            cout << "dirname " << dirpath << endl;
            if (strcmp(dirpath, "/") != 0) {
                string file_path = path_to_shdw_ + (dirpath + 1);
                if (access(file_path.c_str(), F_OK) != 0) {
                    cout << "file path " << file_path << endl;
                    auto parent = meta_data_.find(file_path);
                    mkdir(file_path.c_str(), parent->second->get_mode());
                }
            }
            free(dirpath);
            cout <<"Wew"<<endl;

        }


        // create path to file in shadow dir
        string file_path = path_to_shdw_ + entry.first.substr(1);
        cout << "path to file " << file_path <<  endl;

        // open index file
        int idx_fd = open(idx_path.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
        if (idx_fd == -1)
            perror("Open failed");

        if (entry.second->is_dir()) {
            mkdir(file_path.c_str(), entry.second->get_mode());
            string record = entry.second->get_record();
            ::write(idx_fd, record.c_str(), record.length());
            if (close(idx_fd) == -1)
                cout << "Error closing indx fd Errno " << strerror(errno) << endl;
            continue;
        }
        cout << "alrighty" << endl;

        // open previous version / make new one
        int file_fd = open(file_path.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        if (file_fd == -1)
            cout << "error opening file ERRNO " << strerror(errno) << endl;
        cout << "initiating the flush" << endl;
        entry.second->flush_to_fd(file_fd);
        cout << "finished flush" << endl;

        string record = entry.second->get_record();
        ::write(idx_fd, record.c_str(), record.length());
        if (close(file_fd) == -1)
            cout << "Error closing file ERRNO " << strerror(errno) << endl;
        if (close(idx_fd) == -1)
            cout << "Errror closing indx fd ERRNO " << strerror(errno) << endl;



    }
    meta_data_.clear();
    size_ = 0;

    return 0;
}

vector<string>
BlockCache::get_refs(string path) {
    if(meta_data_.find(path) == meta_data_.end())
        throw domain_error("thing not here");
    return meta_data_.find(path)->second->get_refs();
}
