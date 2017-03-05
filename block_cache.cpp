#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include "includes.h"
using namespace std;

// TODO: testing
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
    has_changed_ = true;
    size_--;
    return 0;
}

int
BlockCache::rmdir(string path) {

    if (in_cache(path) == -1)
        return -1;
    inode_ptrs_[inode_idx_[path]]->delete_inode();
    has_changed_ = true;

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
    has_changed_ = true;

    return 0;
}

int
BlockCache::symlink(string from, string to) {
    shared_ptr<Inode> ll(new Inode(from));
    ll->set_mode(S_IFLNK | S_IRUSR | S_IWUSR);

    inode_idx_[to] = ll->get_id();
    inode_ptrs_[ll->get_id()] = ll;
    has_changed_ = true;

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
    has_changed_ = true;

    return 0;
}

int
BlockCache::load_from_shdw(string path) {
    // TESTING
    int resu = load_from_disk(path);
    return resu;
}

int
BlockCache::getattr(string path, struct stat* st) {
    if ((in_cache(path) == -1 && load_from_shdw(path) == -1)
            || (in_cache(path) == 0 && get_inode_by_path(path)->is_deleted())) {
        cout << "IN CACHE? " << to_string(in_cache(path)) << endl;
        return -ENOENT;
    } else
        return get_inode_by_path(path)->stat(st);
}
/****
 * TODO:
 * fully implement readdir here instead of fuse_ops
 * - iterate thru each root / node file, add every single ent
 * - if it is an updated version
 * - if the latest version is a deletion, it dne
 */
vector<BlockCache::index_entry>
BlockCache::readdir(string path) {
    // map (path, index ent)
    map<string, BlockCache::index_entry> added_names;


    // first check the cache, add stuff that's updated (checking for deletions)
    vector<BlockCache::index_entry> ents;
    for (auto entry : inode_idx_) {
        cout << "Checking " << entry.first << endl;
        char* dirpath = strdup(entry.first.c_str());
        dirpath = dirname(dirpath);
        cout << "DIRNAME " << dirpath << endl;
        if (strcmp(dirpath, path.c_str()) == 0) {
            cout << "ADDING SHIT TO MAP" << endl;
            struct stat st;
            get_inode_by_path(entry.first)->stat(&st);
            index_entry ent;
            ent.path = entry.first;
            ent.deleted = get_inode_by_path(entry.first)->is_deleted();
            ent.added_time = get_inode_by_path(entry.first)->get_ull_mtime();
            added_names[entry.first] = ent;
            // ents.push_back(ent);
        }
        free(dirpath);
    }

    // check stuff on disk
    auto disk_ents = get_all_root_entries();
    for (auto disk_ent : disk_ents) {
        // extract values
        string ent_path = disk_ent.first;
        auto ent_nodes = disk_ent.second;
        //cout << "LOOKING AT |" << ent_path << "|" << endl;
        unsigned long long latest_time = 0;
        // check if path is in dir
        char* dirpath = strdup(ent_path.c_str());
        dirpath = dirname(dirpath);
        if (strcmp(dirpath, path.c_str()) == 0) {
            free(dirpath);
            // iterate thru each .node
            for (auto node_entry : ent_nodes) {
                // record which 1 has the latest entry
                //
                // make path to .node
                string node_name = get<0>(node_entry);
                string path_to_node = path_to_disk_ + node_name;
                // open .node
                int nodefd = ::open(path_to_node.c_str(), O_RDONLY);
                if (nodefd == -1)
                    cout << "ERROR Opening .node ERRNO " << strerror(errno) << endl;
                // get inode info
                uint64_t node_offset = get<2>(node_entry);
                uint64_t node_size = get<3>(node_entry);
                char buf[node_size] = {0};
                if (pread(nodefd, buf, node_size, node_offset) == -1)
                    cout << "ERROR reading .node info ERRNO " << strerror(errno) << endl;

                if (close(nodefd) == -1)
                    cout << "ERROR closing .node ERRNO " << strerror(errno) << endl;
                // skip first line in buf
                string inode_info;
                stringstream ss((string)buf);
                getline(ss, inode_info);
                // interpret contents in buf
                //
                unsigned long long node_mtime = 0;
                int is_deleted = 0;
                sscanf(inode_info.c_str(), "%*s %*d %*d %llu %*d %*d %d", &node_mtime, &is_deleted);
                cout  << "Curtime: " << to_string(latest_time) << " inode mtime " << to_string(node_mtime) << endl;
                if (node_mtime > latest_time) {
                    // add to map if its a later version
                    // make index_entry
                    index_entry ient;
                    ient.path = ent_path;
                    ient.deleted = is_deleted;
                    ient.added_time = node_mtime;

                    added_names[ent_path] = ient;
                    latest_time = node_mtime;
                    cout << "ADDED UPDATED ENTRY" << endl;
                }
            }
        } else {
            free(dirpath);
            continue;
        }
    }
    // temporarily doing this for testing
    for (auto thing : added_names) {
        cout << "ADDED " << thing.first << endl;
        ents.push_back(thing.second);
    }
    // TODO: remove vector for map, can convert back to vec if needed
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
            dirty_block_mtime_[inode_idx_[path]][block_idx] = Util::get_time();

        } else {
            shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));
            // add newly formed block to the inode
            inode->add_block(block_idx, ptr);
            cout << "adding dirty block to map" << endl;
            dirty_block_[inode_idx_[path]][block_idx] = ptr;
            dirty_block_mtime_[inode_idx_[path]][block_idx] = Util::get_time();

        }
    }
    has_changed_ = true;

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
    has_changed_ = true;

    return 0;
}

int
BlockCache::in_cache(string path) {

    return inode_idx_.find(path) != inode_idx_.end()
           || path.compare("/") == 0 ? 0 : -1;
}

int
BlockCache::open(string path) {

    // if (res == 0)
    //     get_inode_by_path(path)->update_atime();

    return 0;
}
//TODO: remove all of this
int
BlockCache::flush_to_shdw(int on_close) {
    //clear_shdw();
    cout << "SIZE " << size_ << endl;
    if (size_ < MAX_SIZE && on_close == 0)
        return -1;
    return flush_to_disk();
}

vector<string>
BlockCache::get_refs(string path) {
    if(in_cache(path) == -1)
        throw domain_error("thing not here");
    return get_inode_by_path(path)->get_refs();
}

int
BlockCache::flush_to_disk() {
    if (!has_changed_)
        return -1;
    lock_guard<mutex> lock(mutex_);
    // create path to .head file
    string fname = Util::generate_rand_hex_name();
    string path_to_node = path_to_disk_ + fname + ".node";
    string path_to_data = path_to_disk_ + fname + ".data";
    string path_to_root = path_to_disk_ + fname + ".root";

    int nodefd = ::open(path_to_node.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int rootfd = ::open(path_to_root.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int datafd = ::open(path_to_data.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    string node_content, root_content, data_content;

    // TODO:
    //  - add checksums for each files

    // create entry for .node file
    uint64_t offset_into_node = 0;
    uint64_t curr_flush_offset = 0;

    // timestamp .root
    string timestamp = to_string(Util::get_time()) + "\n";
    // write to .root
    if (pwrite(rootfd, timestamp.c_str(), timestamp.size() * sizeof(char), 0) == -1)
        cout << "ERROR writing to .root ERRNO: " << strerror(errno) << endl;


    /**
     * This loop writes to the .node and .data files
     * .head file is written to last.
     */
    for (auto ent : inode_idx_) {
        // if nothing was done to this inode, don't flush it
        // TODO: chmod / other ways to modify files aren't gonna
        // be recorded right now, future change!
        // fetch record
        shared_ptr<Inode> flushed_inode = get_inode_by_path(ent.first);
        if (flushed_inode->is_dirty() == 0) {
            cout << "Skipping " << ent.first << " because no changes were made..." << endl;
            continue;
        }


        string inode_data = flushed_inode->get_flush_record();

        string inode_idx = ent.second;

        // NEW!: timestamp this root file
        string root_input;
        // [path] [inode id] [List-of [.node name] [offset into .node] [size-of .node] entry]
        root_input += "INODE: " + ent.first + " " + flushed_inode->get_id() + "\n" + fname + ".node" + " " + to_string(offset_into_node);
        // generate block offset table

        string node_table;
        // write to .data
        // TODO: buffer input for .data and perform single write to it
        for (auto blk : dirty_block_[inode_idx]) {
            cout << "WRITING DIRTY BLOCK" << endl;
            auto block = blk.second;
            uint64_t block_sz = block->get_actual_size();
            auto block_data = block->get_data();
            char buf[block_sz] = {0};
            for (uint64_t ii = 0; ii < block_sz; ii++) {
                buf[ii] = block_data[ii];
            }
            if (pwrite(datafd, buf, block_sz * sizeof(char), curr_flush_offset) == -1)
                cout << "Error flushing block to a file ERRNO " << strerror(errno) << endl;

            node_table += to_string(blk.first) + " "
                          + to_string(curr_flush_offset) + " "
                          + to_string (block_sz) + "\n";
            curr_flush_offset += block_sz;
            cout << "NEW FLUSH OFFSET " << curr_flush_offset << endl;

        }
        uint64_t node_ent_size = node_table.size() + inode_data.size();
        offset_into_node += node_ent_size;

        root_input += " " + to_string(node_ent_size) + "\n";
        // carry over old root entries
        auto old_entries = get_all_root_entries(ent.first)[ent.first];
        if (old_entries.size() > 0) {
            // write them to root_input

            for (auto entry : old_entries) {
                string nname = get<0>(entry);
                uint64_t offset = get<2>(entry);
                uint64_t size = get<3>(entry);
                root_input += nname + " " + to_string(offset) + " " + to_string(size) + "\n";
            }
        }
        node_content += inode_data + node_table;
        root_content += root_input;


    }
    // write to .node
    if (pwrite(nodefd, node_content.c_str(), node_content.size() * sizeof(char), 0) == -1)
        cout << "ERROR writing to .node ERRNO: " << strerror(errno) << endl;

    // write to .root
    if (pwrite(rootfd, root_content.c_str(), root_content.size() * sizeof(char), 0) == -1)
        cout << "ERROR writing to .root ERRNO: " << strerror(errno) << endl;
    close(nodefd);
    close(datafd);
    close(rootfd);
    inode_idx_.clear();
    inode_ptrs_.clear();
    dirty_block_.clear();
    size_ = 0;

    return 0;
}

int
BlockCache::load_from_disk(string path) {
    cout << "LOADING " << path << " FROM DISK" << endl;

    std::shared_ptr<Inode> latest_inode;

    /** map (block idx, block) for this inode */
    map<uint64_t, shared_ptr<Block>> inode_blocks;

    unsigned long long latest_mtime = 0;
    // we have a .root file
    // check if it contains this path
    auto node_files = get_all_root_entries(path)[path];
    if (node_files.size() == 0) {
        // then it doesn't exist in disk
        cout << "NO ROOT ENTRIES" << endl;
        return -1;
    }

    /***
     * node_files has all of the extracted node files for this path and root
     * GOAL AT THE END OF THIS LOOP:
     * - construct an inode in memory based on the latest .node file
     * - extract all of the valid blocks contained in each .node file's .data file
     * node_ent is (node_name, inode id, offset, size)
     ***/
    for (auto node_ent : node_files) {

        // find the .node
        string node_name = get<0>(node_ent);
        string cached_content;
        bool in_cache = false;
        if (meta_cache_.node_content_in_cache(node_name)) {
            // don't need to open, load content into buf
            // to "pread" from
            cached_content = meta_cache_.get_node_file(node_name);
            in_cache = true;

            cout << "NODE IS IN CACHE" << endl;
        }
        string path_to_node = path_to_disk_ + node_name;
        cout << "PATH TO NODE " << path_to_node << endl;

        string inode_id = get<1>(node_ent);
        uint64_t node_offset = get<2>(node_ent);
        uint64_t node_size = get<3>(node_ent);
        char buf[node_size + 1] = {0};
        cout << "NODE ENT SIZE " << to_string(node_size) << " OFFSET " << to_string(node_offset) << endl;
        if (in_cache) {
            // read entry into buf
            memcpy(buf, cached_content.c_str() + node_offset, node_size);

        } else {
            // cache entire .node file, then read from cache
            string node_content = read_entire_file(path_to_node);
            // add node to cache
            meta_cache_.add_node_file(node_name,  node_content);
            // do this only if the file is not in cache
            memcpy(buf, node_content.c_str() + node_offset, node_size);
            //cout << "READ THE FOLLOWING INTO BUF " << buf << endl;

        }

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
        //cout << "FINISHED BUILDING THE FOLLOWING TABLE" << endl;
        /*
        for (auto ent : data_table) {
            cout << "BLOCK NUM " << to_string(ent.first) << " OFFSET# " << to_string(ent.second.first) << " SIZE# " << to_string(ent.second.second) << endl;
        }
        */
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
        string data_name =  node_name.substr(0, node_name.size() -  5) + ".data";

        string path_to_data = path_to_disk_ + data_name;
        cout << "PATH TO DATA " << path_to_data << endl;

        // cache entire data file if it isn't already
        string data_content;
        if (meta_cache_.data_content_in_cache(data_name)) {
            cout << "GOT DATA FILE FROM CACHE" << endl;
            data_content = meta_cache_.get_data_file(data_name);
        } else {
            // cache entire .data file, then read from cache
            cout << "WRITING DATA FILE INTO CACHE" << endl;
            data_content = read_entire_file(path_to_data);
            meta_cache_.add_data_file(data_name, data_content);
        }

        // open the .data, offset and read it
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
            memcpy(data_buf, data_content.c_str() + offset_into_data, size_of_data_ent);

            // add block to map for inode
            inode_blocks[ent.first] = shared_ptr<Block>(new Block(data_buf, size_of_data_ent));

        }
    }

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

    if (!is_updated || in_cache(path) == -1) {
        cout <<" UPDATED THING " << endl;
        inode_idx_[path] = latest_inode->get_id();
        inode_ptrs_[latest_inode->get_id()] = latest_inode;
        latest_inode->undo_dirty();
    }

    return 0;

}

unordered_map<string, vector<tuple<string, string, uint64_t, uint64_t>>>
BlockCache::get_all_root_entries(string path) {

    /** map(path, entries) */
    unordered_map<string, vector<tuple<string, string, uint64_t, uint64_t>>> root_entries;

    /** map of (inode, node) to avoid duplicate entries */
    unordered_map<string, unordered_set<string>> inode_to_node;

    cout << "GETTING ALL ENTRIES FOR " << path << endl;
    /** map (path, root time)
     * if a root file contains a later time than one in here, then the vector of entries is cleared!
     */
    unordered_map<string, unsigned long long> latest_times;
    DIR* root_dir = opendir(path_to_disk_.c_str());
    if (root_dir == NULL) {
        cout << "ERROR opening root DIR ERRNO: " << strerror(errno) << endl;
    }
    struct dirent* entry;
    const char* root_name;
    // iterate thru each entry in root
    while ((entry = ::readdir(root_dir)) != NULL) {
        root_name = entry->d_name;
        // if this file is not a .root, skip it
        if (strlen(root_name) < 4
                || strcmp(root_name + (strlen(root_name) - 5), ".root") != 0) {
            cout << "not a root" << endl;
            continue;
        }

        // it is a root, check if its in cache, if not, add it
        string root_content;
        if (meta_cache_.root_content_in_cache(root_name)) {
            root_content = meta_cache_.get_root_file_contents(root_name);
        } else {
            // make path to .root file
            string path_to_root = path_to_disk_ + root_name;
            // extract all content
            root_content = read_entire_file(path_to_root);
            // add it to cache
            meta_cache_.add_root_file(path_to_root, root_content);
        }
        // check if the contents have been cached
        if (meta_cache_.in_inverted_root_cache(root_name)) {
            auto temp = meta_cache_.get_inverted_root_ent(root_name, path);
            if (temp.size() == 0)
                // its not in this root anyways
                continue;
            // add values to cache
            for (auto ent : temp) {
                for (auto tup : ent.second) {
                    root_entries[ent.first].push_back(tup);
                }
            }

            cout <<"GOT ENT FROM CACHE" << endl;
            continue;

        }
        // for each thing, make a list-of [path, node]
        stringstream ents(root_content);
        string cur_ent;
        bool in_node_table = false;
        string cur_path;
        string cur_id;
        // first get time stamp
        getline(ents, cur_ent);
        unsigned long long root_time;
        sscanf(cur_ent.c_str(), "%llu", &root_time);
        /** map (path, ents)  for cur root*/
        unordered_map<string, vector<tuple<string, string, uint64_t, uint64_t>>> cur_ents;
        cout << "ROOT CONTENTS |" << root_content << "|" << endl;
        while (getline(ents, cur_ent)) {

            cout << "CUR ENT " << cur_ent << endl;
            if (strstr(cur_ent.c_str(), "INODE:") != NULL) {
                // cout << "FOUND AN INODE ENTRY" << endl;
                in_node_table = false;
                /// we have an inode entry, get the path
                char ent_path[PATH_MAX] = {0};
                char ent_id[FILENAME_MAX] = {0};
                sscanf(cur_ent.c_str(), "INODE: %s %s", ent_path, ent_id);
                cur_path = ent_path;
                cur_id = ent_id;
                //  cout << "CUR PATH: " << cur_path << endl;
                //  cout << "PATH: " << path << endl;

                if (path.size() > 0 && cur_path.compare(path) != 0)
                    continue;
                //  cout << "FOUND " << cur_path << endl;
                if (root_time > latest_times[cur_path]) {
                    root_entries[cur_path].clear();
                    cout << "CLEARING OLD ROOT ENTRIES" << endl;
                    latest_times[cur_path] = root_time;
                }

            } else
                in_node_table = true;

            if (in_node_table) {
                char node_name[FILENAME_MAX] = {0};
                uint64_t offset;
                uint64_t size;
                sscanf(cur_ent.c_str(), "%s %" SCNd64 "%" SCNd64, node_name, &offset, &size);
                string nname = (string)node_name;
                if (inode_to_node[cur_path].find(nname) != inode_to_node[cur_path].end())
                    // then we don't need to add this
                    continue;
                auto ent_vals = make_tuple(nname, cur_id, offset, size);
                root_entries[cur_path].push_back(ent_vals);
                inode_to_node[cur_path].insert(nname);
                cout << "ADDED ENTRY TO MAP FOR |" << cur_path << "|" << endl;
                cur_ents[cur_path].push_back(ent_vals);
            }
        }
        // cache stuff
        if (path.compare("") == 0
                && !meta_cache_.in_inverted_root_cache(root_name)) {
            for (auto ent : cur_ents) {
                meta_cache_.add_inverted_root_entry(root_name, ent.first, ent.second);
            }
            cout <<"CACHED ROOT STUFF"<<endl;
        }
    }
    if (root_dir != NULL)
        closedir(root_dir);

    return root_entries;
}

string
BlockCache::read_entire_file(string path) {

    ifstream ifs(path);
    stringstream buffer;
    buffer << ifs.rdbuf();

    return buffer.str();
}
