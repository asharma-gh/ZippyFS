#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
using namespace std;
/**
 * TODO:
 * other i/o stuff, deletions
 * integrate inodes
 *
 */
BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {}

int
BlockCache::remove_link(string path) {
    if (meta_data_.find(path) == meta_data_.end())
        return -1;
    meta_data_[path]->dec_link();
    return 0;
}

int
BlockCache::make_file(string path, mode_t mode) {
    shared_ptr<Inode> ptr(new Inode(path));
    ptr->set_mode(mode);
    meta_data_[path] = ptr;
    return 0;
}

int
BlockCache::load_from_shdw(string path) {
    // construct path to shdw
    string shdw_file_path = path_to_shdw_ + path.substr(1);

    // open, extract bytes
    FILE* file;
    if ((file = fopen(shdw_file_path.c_str(), "r")) == NULL) {
        return -1;
    }
    // get file size
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char contents[fsize + 1];
    memset(contents, 0, sizeof(contents) / sizeof(char));
    fread(contents, fsize, 1, file);
    contents[fsize] = '\0';
    fclose(file);

    // add this file to cache
    write(path, (uint8_t*)contents, fsize, 0);
    return 0;
}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    cout << "SIZE " << size << " OFFSET " << offset << endl;
    // create blocks for buf
    uint64_t num_blocks = Util::ulong_ceil(size + offset, Block::get_logical_size());
    /*** create inode for write if needed ***/
    shared_ptr<Inode> inode;
    if (meta_data_.find(path) != meta_data_.end()) {
        inode = meta_data_[path];
        inode->set_mode(S_IRUSR | S_IWUSR);
    } else {
        inode = make_shared<Inode>(path);
    }
    if (inode->get_size() < size + offset)
        inode->set_size(size + offset);

    // for this file, make a block and add it to cache
    uint64_t curr_idx = 0;
    uint64_t block_size = 0;
    for (unsigned int block_idx = offset / Block::get_logical_size();  block_idx < num_blocks; block_idx++) {
        curr_idx += block_size;
        if (size < curr_idx + Block::get_logical_size())
            block_size = size - curr_idx;
        else
            block_size = Block::get_logical_size();

        // finally create block with that much space at the current byte
        shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));
        // add newly formed block to the inode
        inode->add_block(block_idx, ptr);
    }
    assert(curr_idx + block_size == size);

    // record meta data to cache_data
    // get prev inode if it exists
    meta_data_[path] = inode;
    return size;
}

int
BlockCache::read(string path, uint8_t* buf, uint64_t size, uint64_t offset) {
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

bool
BlockCache::in_cache(string path) {
    (void)path;
    return meta_data_.find(path) != meta_data_.end();
}

int
BlockCache::flush_to_shdw() {
    // make index file for cache
    string idx_path = path_to_shdw_ + "index.idx";
    cout << "PATH TO SHDW " << idx_path << endl;

    // create files for each item in cache
    for (auto const& entry : meta_data_) {
        // load previous version to shadow director

        load_to_shdw(entry.first.c_str());
        // create path to file in shadow dir
        string shdw_file_path = path_to_shdw_ + (entry.first).substr(1);
        // open previous version / make new one
        int file_fd = open(shdw_file_path.c_str(), O_CREAT | O_WRONLY);
        entry.second->flush_to_fd(file_fd);

        int idx_fd = open(idx_path.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
        if (idx_fd == -1)
            perror("Open failed");
        string record = entry.second->get_record();
        int res = ::write(idx_fd, record.c_str(), record.length());
        cout << "RES: " << res << endl;
        cout << "fd: " << idx_fd << endl;
        // cout << "ERRNO: " << strerror(errno) << endl;
        close(idx_fd);



    }
    // meta_data__.clear();
    return 0;
}

vector<string>
BlockCache::get_refs(string path) {
    if(meta_data_.find(path) == meta_data_.end())
        throw domain_error("thing not here");
    return meta_data_.find(path)->second->get_refs();
}
