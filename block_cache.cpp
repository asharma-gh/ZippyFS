#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include "includes.h"
using namespace std;

// TODO: get rid of shadow directory
BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

    path_to_disk_ = "/home/arvin/FileSystem/zipfs/o/dir/root/";
}

BlockCache::BlockCache(string path_to_disk, bool f) :
    path_to_disk_(path_to_disk) {
    (void)f;
}

int
BlockCache::remove(string path) {
    if (in_cache(path) == -1)
        return -1;

    for (auto entry : inode_ptrs_) {
        auto vec = entry.second->get_refs();
        if (find(vec.begin(), vec.end(), path) != vec.end())
            entry.second->dec_link(path);
    }
    get_inode_by_path(path)->delete_inode();
    size_--;
    return 0;
}

int
BlockCache::rmdir(string path) {

    if (in_cache(path) == -1)
        return -1;
    inode_ptrs_[inode_idx_[path]]->delete_inode();
    return 0;
}

int
BlockCache::rename(string from, string to) {
    int res = 0;

    if (in_cache(from) == -1)
        res = load_from_shdw(from);

    if (res == -1)
        return -1;

    if (in_cache(to) == -1)
        res = load_from_shdw(to);

    if (res == -1)
        make_file(to, get_inode_by_path(from)->get_mode(), 1);
    shared_ptr<Inode> nto_inode(new Inode(to, *get_inode_by_path(from)));
    get_inode_by_path(from)->delete_inode();
    inode_idx_[to] = nto_inode->get_id();
    inode_ptrs_[inode_idx_[to]] = nto_inode;
    return 0;
}

int
BlockCache::symlink(string from, string to) {
    shared_ptr<Inode> ll(new Inode(from));
    ll->set_mode(S_IFLNK | S_IRUSR | S_IWUSR);

    inode_idx_[to] = ll->get_id();
    inode_ptrs_[ll->get_id()] = ll;

    return 0;
}

int
BlockCache::readlink(std::string path, uint8_t* buf, uint64_t size) {
    if (in_cache(path) == -1
            || !S_ISLNK(get_inode_by_path(path)->get_mode()))
        return -ENOENT;
    else {
        return get_inode_by_path(path)->read(buf, size, 0);
    }


}

shared_ptr<Inode>
BlockCache::get_inode_by_path(string path) {
    return inode_ptrs_[inode_idx_[path]];
}

int
BlockCache::make_file(string path, mode_t mode, bool dirty) {
    shared_ptr<Inode> ptr(new Inode(path));
    ptr->set_mode(mode);
    if (dirty)
        ptr->set_dirty();
    size_++;
    inode_idx_[path] = ptr->get_id();
    inode_ptrs_[ptr->get_id()] = ptr;
    return 0;
}

int
BlockCache::load_from_shdw(string path) {
    // TESTING
    int resu = load_from_disk(path);
    return resu;
    cout << "loading to cache from shdw " << path << endl;
    int res = load_to_shdw(path.c_str());
    // construct path to shdw
    string shdw_file_path = path_to_shdw_ + path.substr(1);
    cout << "path to shdw file " << shdw_file_path << endl;

    cout << "RESULT " << res << endl;
    if ((res == -1 || res == 1) && in_cache(path) == -1) {
        cout << "could not find the thing " << endl;
        return -1;
    }
    // compare times of thing in cache and entry
    // if both exist
    struct stat st;
    struct stat ino_st;
    stat(shdw_file_path.c_str(), &st);

    if (res == 0 && in_cache(path) == 0) {
        get_inode_by_path(path)->stat(&ino_st);
        cout << "time dif " << to_string(difftime(st.st_mtim.tv_sec, ino_st.st_mtim.tv_sec)) << endl;
        if (difftime(st.st_mtim.tv_sec, ino_st.st_mtim.tv_sec) < 1) {
            cout << "UPDATED VERSION IS IN CACHE " << endl;
            // updated version in here
            return 0;
        }
    }
    if ((res == -1 || res == 1) && in_cache(path) == 0) {
        cout << "this thing has been in cache ever" << endl;
        return 0;
    }

    cout << " thing is in shdw" << endl;

    struct stat shdw_st;
    stat(shdw_file_path.c_str(), &shdw_st);

    if (S_ISDIR(shdw_st.st_mode)) {
        cout << "made dir" << endl;
        make_file(path, shdw_st.st_mode, 0);
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
    make_file(path, st.st_mode, 0);
    write(path, (uint8_t*)contents, fsize, 0);
    get_inode_by_path(path)->set_st_time(shdw_st.st_mtim, shdw_st.st_ctim);
    get_inode_by_path(path)->undo_dirty();
    cout << "FLIPPED DIRTY " << endl;
    cout << "finished loading to shdw" << endl;
    return 0;
}

int
BlockCache::getattr(string path, struct stat* st) {
    if ((in_cache(path) == -1 && load_from_shdw(path) == -1)
            || (in_cache(path) == 0 && get_inode_by_path(path)->is_deleted()))
        return -ENOENT;
    else
        return get_inode_by_path(path)->stat(st);
}

vector<BlockCache::index_entry>
BlockCache::readdir(string path) {
    vector<BlockCache::index_entry> ents;
    for (auto entry : inode_idx_) {
        char* dirpath = strdup(entry.first.c_str());
        dirpath = dirname(dirpath);
        if (strcmp(dirpath, path.c_str()) == 0) {
            struct stat st;
            get_inode_by_path(entry.first)->stat(&st);
            index_entry ent;
            ent.path = entry.first;
            ent.deleted = get_inode_by_path(entry.first)->is_deleted();
            ent.added_time = get_inode_by_path(entry.first)->get_ull_mtime();
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

    } else {
        get_inode_by_path(path)->set_dirty();
        get_inode_by_path(path)->remake_inode();
        // if file was deleted, now it ain't
    }
    // create blocks for buf
    uint64_t num_blocks = Util::ulong_ceil(size + offset, Block::get_physical_size());
    /*** create inode for write if needed ***/
    shared_ptr<Inode> inode;
    inode = get_inode_by_path(path);
    if (inode->get_size() < size + offset)
        inode->set_size(size + offset);

    // for this file, make a block and add it to cache
    uint64_t curr_idx = 0;
    uint64_t block_size = 0;
    bool offsetted = false;
    for (unsigned int block_idx = offset / Block::get_physical_size();  block_idx < num_blocks; block_idx++) {

        curr_idx += block_size;

        if (size < curr_idx + Block::get_physical_size())
            block_size = size - curr_idx;
        else
            block_size = Block::get_physical_size();

        int offset_amt = 0;
        if (offsetted == false) {
            offset_amt = offset < Block::get_physical_size() ? offset :
                         (offset % Block::get_physical_size());
            offsetted = true;
        }
        cout << "block idx " << block_idx << endl;
        if (inode->has_block(block_idx)) {
            cout << "WE HAVE THE BLOCK" << endl;
            shared_ptr<Block> temp = inode->get_block(block_idx);
            temp->insert(buf, block_size, offset_amt);
            cout << "overrided block" << endl;
            cout << "adding dirty block to map" << endl;
            dirty_block_[inode_idx_[path]][block_idx] = temp;

        } else {
            shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));
            // add newly formed block to the inode
            inode->add_block(block_idx, ptr);
            cout << "adding dirty block to map" << endl;
            dirty_block_[inode_idx_[path]][block_idx] = ptr;
        }
    }
    assert(curr_idx + block_size == size);
    return size;
}

int
BlockCache::read(string path, uint8_t* buf, uint64_t size, uint64_t offset) {
    if (in_cache(path) == -1) {
        cout << " READ: loading from shdw " << path << endl;
        load_from_shdw(path);
    }
    return get_inode_by_path(path)->read(buf, size, offset);
}

int
BlockCache::truncate(string path, uint64_t size) {
    if (in_cache(path) == -1)
        return -1;

    get_inode_by_path(path)->set_size(size);
    return 0;
}

int
BlockCache::in_cache(string path) {

    return inode_idx_.find(path) != inode_idx_.end()
           || path.compare("/") == 0 ? 0 : -1;
}

int
BlockCache::open(string path) {
    int res = load_from_shdw(path);
    cout << "open res " << res << endl;
    if (res == 0)
        get_inode_by_path(path)->update_atime();

    return 0;
}

int
BlockCache::flush_to_shdw(int on_close) {
    clear_shdw();
    cout << "SIZE " << size_ << endl;
    if (size_ < MAX_SIZE && on_close == 0)
        return -1;
    flush_to_disk();

    // make index file for cache
    string idx_path = path_to_shdw_ + "index.idx";
    cout << "PATH TO SHDW " << idx_path << endl;

    // create files for each item in cache
    for (auto const& entry : inode_idx_) {
        // load previous version to shadow director
        cout  << "NAME " << entry.first << endl;
        cout << "DIRTY? " << get_inode_by_path(entry.first)->is_dirty() << endl;
        shared_ptr<Inode> ent = get_inode_by_path(entry.first);
        if (ent->is_dirty() == 0)
            continue;
        cout << "IS NOT DIRTY" << endl;

        cout << "==== PRINTING OUT FLUSH RECORD ====" << endl;
        cout << ent->get_flush_record() << endl;
        // open index file
        int idx_fd = ::open(idx_path.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
        if (idx_fd == -1)
            perror("Open failed");

        if (ent->is_deleted()) {
            string record = ent->get_record();
            ::write(idx_fd, record.c_str(), record.length());
            close(idx_fd);
            continue;

        }

        // load file or parent to shdw
        int res = load_to_shdw(entry.first.c_str());
        if (res == -1) {
            char* dirpath = strdup(entry.first.c_str());
            cout << "dirp " << dirpath << endl;
            dirpath = dirname(dirpath);
            cout << "dirname " << dirpath << endl;
            if (strcmp(dirpath, "/") != 0) {
                string file_path = path_to_shdw_ + (dirpath + 1);
                cout << "file path " << file_path << endl;
                if (in_cache(file_path) == 0)
                    mkdir(file_path.c_str(), get_inode_by_path(file_path)->get_mode());

            }
            free(dirpath);
        }

        // create path to file in shadow dir
        string file_path = path_to_shdw_ + entry.first.substr(1);
        cout << "path to file " << file_path <<  endl;

        if (ent->is_dir()) {
            mkdir(file_path.c_str(), ent->get_mode());
            string record = ent->get_record();
            ::write(idx_fd, record.c_str(), record.length());
            if (close(idx_fd) == -1)
                cout << "Error closing indx fd Errno " << strerror(errno) << endl;
            continue;
        }

        // open previous version / make new one
        int file_fd = ::open(file_path.c_str(), O_CREAT | O_WRONLY, ent->get_mode());
        //cout << "MODE " << ent->get_mode() << endl;
        if (file_fd == -1)
            cout << "error opening file ERRNO " << strerror(errno) << endl;
        cout << "initiating the flush" << endl;
        ent->flush_to_fd(file_fd);
        cout << "finished flush" << endl;

        string record = ent->get_record();
        ::write(idx_fd, record.c_str(), record.length());
        if (close(file_fd) == -1)
            cout << "Error closing file ERRNO " << strerror(errno) << endl;
        if (close(idx_fd) == -1)
            cout << "Errror closing indx fd ERRNO " << strerror(errno) << endl;

    }
    inode_idx_.clear();
    inode_ptrs_.clear();
    dirty_block_.clear();
    size_ = 0;
    flush_dir();
    return 0;
}

vector<string>
BlockCache::get_refs(string path) {
    if(in_cache(path) == -1)
        throw domain_error("thing not here");
    return get_inode_by_path(path)->get_refs();
}

int
BlockCache::flush_to_disk() {
    // create path to .head file
    string fname = Util::generate_rand_hex_name();
    string path_to_node = path_to_disk_ + fname + ".node";
    string path_to_data = path_to_disk_ + fname + ".data";
    string path_to_root = path_to_disk_ + fname + ".root";

    int nodefd = ::open(path_to_node.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int rootfd = ::open(path_to_root.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int datafd = ::open(path_to_data.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);


    // TODO: look for previous latest .index for the current file
    //  - read each .head file, looking for this inode idx
    //  - record each (.node, offset) pair
    //
    //  - add checksums for each files

    // create entry for .node file
    uint64_t offset_into_data = 0;
    uint64_t offset_into_node = 0;

    /** the input for the .root file.
     * Every "flush" constructs a new root
     * TODO: have this root contain previous .index / .nodes
     */


    /**
     * This loop writes to the .node and .data files
     * .head file is written to last.
     */
    for (auto ent : inode_idx_) {
        // fetch record
        shared_ptr<Inode> flushed_inode = get_inode_by_path(ent.first);
        string inode_data = flushed_inode->get_flush_record();
        // get offset map
        auto offst_mp = get_offsets(ent.second);

        string root_input;
        // [path] [inode id] [.node name] [offset into .node] [size-of .node] entry
        root_input += ent.first + " " + flushed_inode->get_id() + " " + fname + ".node" + " " + to_string(offset_into_node);
        // generate block offset table

        unordered_map<uint64_t, pair<uint64_t,uint64_t>> updated_mp;
        for (auto ent : offst_mp.second) {
            updated_mp[ent.first] = pair<uint64_t, uint64_t>(ent.second.first + offset_into_data, ent.second.second);
        }
        offset_into_data += offst_mp.first;
        string table;
        for (auto ent : updated_mp) {
            table += to_string(ent.first) + " "
                     + to_string(ent.second.first) + " "
                     + to_string(ent.second.second) + "\n";
        }
        //compute offset into node, node entry size
        uint64_t node_ent_size = table.size() + inode_data.size();
        offset_into_node += node_ent_size;

        // write to root
        root_input += " " + to_string(node_ent_size) + "\n";

        // write to .node
        if (pwrite(nodefd, inode_data.c_str(), inode_data.size() * sizeof(char), 0) == -1)
            cout << "ERROR writing to .node ERRNO: " << strerror(errno) << endl;
        if (pwrite(nodefd, table.c_str(), table.size() * sizeof(char), 0) == -1)
            cout << "ERROR writing TABLE to .node ERRNO: " << strerror(errno);
        // write to .data
        // TODO: move away from this! Only flush dirty blocks, partition off the data file for each
        // inode to avoid data corruption, which will occur using this!
        flushed_inode->flush_to_fd(datafd);

        // write to .root
        if (pwrite(rootfd, root_input.c_str(), root_input.size() * sizeof(char), 0) == -1)
            cout << "ERROR writing to .index ERRNO: " << strerror(errno) << endl;

    }

    close(nodefd);
    close(datafd);
    close(rootfd);

    return 0;
}

int
BlockCache::load_from_disk(string path) {
    cout << "LOADING " << path << " FROM DISK" << endl;

    // find latest root with the given file
    //uint64_t latest_time = 0;
    //string latest_file;
    // open directory of roots
    DIR* root_dir = opendir(path_to_disk_.c_str());
    if (root_dir == NULL) {
        cout << "ERROR opening root DIR ERRNO: " << strerror(errno) << endl;
        return -1;
    }
    std::shared_ptr<Inode> latest_inode;

    /** map (block idx, block) for this inode */
    map<uint64_t, shared_ptr<Block>> inode_blocks;

    unsigned long long latest_mtime = 0;
    struct dirent* entry;
    const char* entry_name;
    // iterate thru each entry in root
    while ((entry = ::readdir(root_dir)) != NULL) {
        entry_name = entry->d_name;

        // if this file is not a .root, skip it
        if (strlen(entry_name) < 4
                || strcmp(entry_name + (strlen(entry_name) - 5), ".root") != 0)
            continue;

        // we have a .root file
        // check if it contains this path
        auto node_files = find_entry_in_root(entry_name, path);
        if (node_files.size() == 0)
            // then it is not in this root file, skip it
            continue;
        /***
         * node_files has all of the extracted node files for this path and root
         * GOAL AT THE END OF THIS LOOP:
         * - construct an inode in memory based on the latest .node file
         * - extract all of the valid blocks contained in each .node file's .data file
         * node_ent is (node_name, inode id, offset, size)
         ***/
        for (auto node_ent : node_files) {

            //TODO: stuff with the data in the .head file
            //probably need to keep track of blocks so we only load one if it is a later version

            // find the .node
            string node_name = get<0>(node_ent);
            string path_to_node = path_to_disk_ + node_name;
            cout << "PATH TO NODE " << path_to_node << endl;

            // open the .node, offset and read it
            int nodefd = ::open(path_to_node.c_str(), O_RDONLY);
            if (nodefd == -1)
                cout << "ERROR opening .node file at " << path_to_node << " ERRNO " << strerror(errno) << endl;
            string inode_id = get<1>(node_ent);
            uint64_t node_offset = get<2>(node_ent);
            uint64_t node_size = get<3>(node_ent);
            char buf[node_size + 1] = {0};
            cout << "NODE ENT SIZE " << to_string(node_size) << " OFFSET " << to_string(node_offset) << endl;
            if (pread(nodefd, buf, node_size, node_offset) == -1)
                cout << "ERROR reading .node entry ERRNO " << strerror(errno) << endl;
            cout << "READ THE FOLLOWING INTO BUF " << buf << endl;

            if (close(nodefd) == -1)
                cout << "ERROR closing .node file ERRNO " << strerror(errno) << endl;

            string node_contents = (string)buf;
            cout << "CONTENTS " << node_contents << endl;
            // interpret node entry
            istringstream sstream(node_contents);
            string inode_info;
            // first line is always inode info
            getline(sstream, inode_info);

            string table_ent;

            /** map of (block#, (offset into data, size)) */
            map<uint64_t, pair<uint64_t, uint64_t>> data_table;

            // build table
            while (getline(sstream, table_ent)) {
                uint64_t blockidx, data_offset, block_size = 0;
                cout << "TABLE ENTRY "  << table_ent << endl;
                sscanf(table_ent.c_str(), "%" SCNd64" %" SCNd64" %" SCNd64, &blockidx, &data_offset, &block_size);

                data_table[blockidx] = make_pair(data_offset, block_size);
            }
            cout << "INODE INFO " << inode_info << endl;
            cout << "FINISHED BUILDING THE FOLLOWING TABLE" << endl;

            for (auto ent : data_table) {
                cout << "BLOCK NUM " << to_string(ent.first) << " OFFSET# " << to_string(ent.second.first) << " SIZE# " << to_string(ent.second.second) << endl;
            }
            // we now have the entire inode entry
            // record inode data
            //
            // interpret inode_info
            char inode_path[PATH_MAX];
            uint32_t mode, nlinks = 0;
            unsigned long long mtime, ctime = 0;
            uint64_t size = 0;
            int is_deleted = 0;
            sscanf(inode_info.c_str(), "%s %" SCNd32 " %" SCNd32 " %llu %llu %" SCNd64 "%d",
                   inode_path, &mode, &nlinks, &mtime, &ctime, &size, &is_deleted);

            // make inode if this is a later version
            bool updated = false;
            if (mtime > latest_mtime) {
                latest_mtime = mtime;
                updated = true;
                latest_inode = shared_ptr<Inode>(new Inode(path));
                latest_inode->set_id((string)inode_id);
                latest_inode->set_size(size);
                latest_inode->set_mode(mode);
                latest_inode->set_nlink(nlinks);
                latest_inode->set_mtime(mtime);
                latest_inode->set_ctime(ctime);
                if (is_deleted) {
                    latest_inode->delete_inode();
                    // if this entry was a deletion, then there is no data to read
                    continue;
                }
            }

            // find the .data, open it
            string path_to_data = path_to_disk_ + node_name.substr(0, node_name.size() -  5) + ".data";
            cout << "PATH TO DATA " << path_to_data << endl;

            // open the .data, offset and read it
            int datafd = ::open(path_to_data.c_str(), O_RDONLY);
            if (datafd == -1)
                cout << "ERROR OPENING .data file ERRNO " << strerror(errno) << endl;
            for (auto ent : data_table) {
                // if this isn't an updated inode and we already have a block, then don't add it!
                // else we either have an updated inode or do not have the inode block, so we add it.
                if (!updated && inode_blocks.find(ent.first) != inode_blocks.end())
                    continue;

                // ent.first is the block#, ent.second.first is offset#, ent.second.second is size#
                uint64_t offset_into_data = ent.second.first;
                uint64_t size_of_data_ent = ent.second.second;
                uint8_t data_buf[size_of_data_ent] = {0};
                // read data, add to map
                if (pread(datafd, data_buf, size_of_data_ent, offset_into_data) == -1)
                    cout << "ERROR reading data in data file ERRNO " << strerror(errno) << endl;

                // add block to map for inode
                inode_blocks[ent.first] = shared_ptr<Block>(new Block(data_buf, size_of_data_ent));


            }
            if (close(datafd) == -1)
                cout << "ERROR closing data file ERRNO " << strerror(errno) << endl;
        }
    }
    closedir(root_dir);

    // if latest_mtime is still 0, then we could not find an inode for this path
    if (latest_mtime == 0) {
        cout << "Time didn't change!" << endl;
        return -1;
    }

    // add data to the latest inode
    for (auto ent : inode_blocks) {
        latest_inode->add_block(ent.first, ent.second);
    }

    // add to cache if this one is a later version than the current one, if there is a current one
    bool is_updated = (in_cache(path) == 0) && get_inode_by_path(path)->get_ull_mtime() > latest_mtime;

    if (!is_updated) {
        cout <<" UPDATED THING " << endl;
        inode_idx_[path] = latest_inode->get_id();
        inode_ptrs_[latest_inode->get_id()] = latest_inode;
    }

    return 0;

}

vector<tuple<string, string, uint64_t, uint64_t>>
BlockCache::find_entry_in_root(string root_name, string path) {
    vector<tuple<string, string, uint64_t, uint64_t>> node_files;
    string ab_root_path = path_to_disk_ + root_name;
    // iterate thru each entry in this root file
    ifstream in_file(ab_root_path);
    string curline;
    while (getline(in_file, curline)) {
        char cur_path[PATH_MAX];
        char node_ent[FILENAME_MAX];
        char inode_id[FILENAME_MAX];
        uint64_t offset, size = 0;
        sscanf(curline.c_str(), "%s %s %s %" SCNd64 " %" SCNd64, cur_path, inode_id, node_ent, &offset, &size);
        if (strcmp(cur_path, path.c_str()) == 0) {
            node_files.push_back(make_tuple((string)node_ent, (string)inode_id, offset, size));
        }
    }
    cout << "NUMBER OF NODE FILES " << to_string(node_files.size()) << endl;
    return node_files;
}


pair<uint64_t, map<uint64_t, pair<uint64_t, uint64_t>>>
BlockCache::get_offsets(string inode_idx) {
    uint64_t curr_offset = 0;
    /** map of (block idx, offset into data) for record */
    map<uint64_t, pair<uint64_t, uint64_t>> offsets_for_blocks;

    for (auto ent : dirty_block_[inode_idx]) {
        auto bl_data = ent.second->get_data();
        offsets_for_blocks[ent.first] = pair<uint64_t, uint64_t>(curr_offset, ent.second->get_actual_size());

        curr_offset += ent.second->get_actual_size();
    }
    for (auto ent : offsets_for_blocks) {
        cout << "Block # " << ent.first << " Offset # " << ent.second.first << endl;
    }

    return pair<uint64_t, map<uint64_t, pair<uint64_t, uint64_t>>>(curr_offset, offsets_for_blocks);

}

void
BlockCache::flush_dirty_blocks(int datafd) {
    (void)datafd;
    // iterate thru each dirty block
    // write at block idx * logical size
}
