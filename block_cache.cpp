#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include "includes.h"
using namespace std;

// TODO: testing
BlockCache::BlockCache(string path_to_disk) {
    path_to_disk_ = path_to_disk + "root/";
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

    unsigned long long time = Util::get_time();
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
    auto disk_ents = get_all_root_entries("",path);
    unsigned long long dtime = Util::get_time();
    for (auto disk_ent : disk_ents) {
        // extract values
        string ent_path = disk_ent.first;
        auto ent_nodes = disk_ent.second;
        //cout << "LOOKING AT |" << ent_path << "|" << endl;
        unsigned long long latest_time = 0;
        // check if path is in dir
        //cout << "PATH: " << ent_path << endl;
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
            if (!dirpath)
                free(dirpath);
            continue;
        }
    }
    // temporarily doing this for testing
    for (auto thing : added_names) {
        cout << "ADDED " << thing.first << endl;
        ents.push_back(thing.second);
    }
    unsigned long long etime = Util::get_time();

    cout << "\nDiff start & end: " << to_string (etime - dtime)
         << "\nDiff disk & end: " << to_string (etime - time)
         << "\nDiff start & disk: " << to_string (dtime - time)
         << endl;
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
    (void)path;
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
/**
 * TODO:
 * - move .node info into the .meta files
 * - write a .data for every .meta
 */
int
BlockCache::flush_to_disk() {
    if (!has_changed_)
        return -1;

    /** map(path, (.file name, content)) */
    unordered_map<string, pair<string, string>> flushed_file_ents;

    /** map(path, (.file name, data)) */
    unordered_map<string, pair<string, string>> flushed_file_data;

    /**
     * This loop writes to the .node and .data files
     * .head file is written to last.
     */
    for (auto ent : inode_idx_) {
        shared_ptr<Inode> flushed_inode = get_inode_by_path(ent.first);
        if (flushed_inode->is_dirty() == 0) {
            cout << "Skipping " << ent.first << " because no changes were made..." << endl;
            continue;
        }

        // file name for this inode
        string file_name = Util::generate_fname(ent.first);

        string inode_idx = ent.second;
        string root_input;
        // [path] [inode id] [mode] [#links] [mtime] [ctime] [size] deleted\n
        // [block offset table]\n
        root_input += "INODE: " + flushed_inode->get_flush_record();
        // generate block offset table
        // and gen block data for file
        string data_entry;
        string data_table;
        uint64_t cur_offset = 0;
        cout << "Flushing " << ent.first << endl;
        for (auto blck : dirty_block_[inode_idx]) {
            auto block = blck.second;
            uint64_t block_sz = block->get_actual_size();
            auto block_data = block->get_data();
            char buf[block_sz + 1] = {'\0'};
            for (unsigned int ii = 0; ii < block_sz; ii++)
                buf[ii] = block_data[ii];
            data_entry += buf;
            data_table += to_string(blck.first) + " "
                          + to_string(cur_offset) + " "
                          + to_string(block_sz) + "\n";
            cur_offset += block_sz;
        }
        root_input += data_table;

        flushed_file_ents[ent.first] = make_pair(file_name, root_input);
        flushed_file_data[ent.first] = make_pair(Util::generate_dataname(file_name), data_entry);

    }


    // write .files
    for (auto ent : flushed_file_ents) {
        auto file = ent.second;
        string fpath = path_to_disk_ + file.first;

        int fd = ::open(fpath.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
        if (pwrite(fd, file.second.c_str(), file.second.size() * sizeof(char), 0) == -1)
            cout << "Error writing to .meta ERRNO " << strerror(errno) << endl;
        auto data_ent = flushed_file_data[ent.first];
        string dataf = path_to_disk_ + data_ent.first;
        int datafd = ::open(dataf.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);

        if (pwrite(datafd, data_ent.second.c_str(), data_ent.second.size() * sizeof(char), 0) == -1)
            cout << "Error writing to .data ERRNO " << strerror(errno) << endl;
        close(datafd);
        close(fd);

    }

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
    auto node_files = get_all_root_entries(path, "")[path];
    if (node_files.size() == 0) {
        // then it doesn't exist in disk
        cout << "NO ROOT ENTRIES" << endl;
        return -1;
    }
    return 0;

    // DEPRECATED!!! REMOVING NODE FILES !!!
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

        string path_to_node = path_to_disk_ + node_name;
        cout << "PATH TO NODE " << path_to_node << endl;

        string inode_id = get<1>(node_ent);
        uint64_t node_offset = get<2>(node_ent);
        uint64_t node_size = get<3>(node_ent);
        char buf[node_size + 1] = {0};
        cout << "NODE ENT SIZE " << to_string(node_size) << " OFFSET " << to_string(node_offset) << endl;
        int nfd = ::open (path_to_node.c_str(), O_RDONLY);
        if (pread(nfd, buf, node_size, node_offset) == -1)
            cout << "Error reading node ERRNO " << strerror(errno) << endl;

        close(nfd);



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

        int datafd = ::open(path_to_data.c_str(), O_RDONLY);
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
            if (pread(datafd, data_buf, size_of_data_ent, offset_into_data) == -1)
                cout << "Error reading data ERRNO: " << strerror(errno) << endl;
            // add block to map for inode
            inode_blocks[ent.first] = shared_ptr<Block>(new Block(data_buf, size_of_data_ent));
        }
        close(datafd);
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
BlockCache::get_all_root_entries(string path, string parent) {
    // TODO: cache to avoid reads
    /** map(path, entries) */
    unordered_map<string, vector<tuple<string, string, uint64_t, uint64_t>>> root_entries;

    /** map of (inode, node) to avoid duplicate entries */
    unordered_map<string, unordered_set<string>> inode_to_node;
    string hashname;
    string pattern;
    glob_t res;
    unsigned long long start = Util::get_time();
    if (path.size() > 0) {
        cout << "FINDING MATCHING META FILES..." << endl;
        // add - to end to avoid collision w/ rand section
        hashname = "*." + Util::crypto_hash(path) + "-";
        pattern = path_to_disk_ + hashname + "*";
    } else if (path.size() == 0 && parent.size() > 0) {
        if (parent.size() == 1)
            parent = "ROOT";

        hashname = Util::crypto_hash(parent);
        pattern  = path_to_disk_ + hashname + ".*";
    } else {
        cout << "Nothing to do..." << endl;
        return root_entries;
    }
    glob(pattern.c_str(), GLOB_NOSORT, NULL, &res);
    cout << "SIZE: " << to_string(res.gl_pathc) << endl;
    if (res.gl_pathc == 0)
        return root_entries;

    unsigned long long dstart = Util::get_time();
    cout << "Checking thru stuff" << endl;
    // check thru glob stuff
    for (uint64_t ii = 0; ii < res.gl_pathc; ii++) {
        string content = read_entire_file(res.gl_pathv[ii]);
        cout << "Content: " << content << endl;
        stringstream sstream(content);
        char ent_path[PATH_MAX] = {0};
        char ent_id[FILENAME_MAX] = {0};
        string cur_ent;
        getline(sstream, cur_ent);
        sscanf(cur_ent.c_str(), "INODE: %s %s", ent_path, ent_id);

        while(getline(sstream, cur_ent)) {

            cout << "Stuff: " << cur_ent << endl;
            // everything else must be in the ent table
            char node_name[FILENAME_MAX] = {0};
            uint64_t offset;
            uint64_t size;
            sscanf(cur_ent.c_str(), "%s %" SCNd64 "%" SCNd64, node_name, &offset, &size);
            string nname = (string)node_name;
            auto ent_vals = make_tuple(nname, ent_id, offset, size);
            if (inode_to_node[ent_path].find(nname) != inode_to_node[ent_path].end())
                // then we don't need to add this
                continue;
            root_entries[ent_path].push_back(ent_vals);
            inode_to_node[ent_path].insert(nname);
        }

    }

    unsigned long long end = Util::get_time();
    cout << "DIF FROM START " << to_string(end - start)
         << endl;
    cout << "DIF FROM DISK " << to_string (end  - dstart)
         << endl;
    cout << "DIF OF START TO DISK: " << to_string(dstart - start)
         << endl;
    return root_entries;

}

string
BlockCache::read_entire_file(string path) {

    ifstream ifs(path);
    stringstream buffer;
    buffer << ifs.rdbuf();

    return buffer.str();
}

string
BlockCache::get_latest_meta(string path) {
    auto ents = get_all_root_entries(path, "");
    return path;
}

unordered_set<string>
BlockCache::get_all_meta_files(string path, bool is_parent) {
    (void)path;
    (void)is_parent;
    unordered_set<string> meta_files;
    string hashname;
    string pattern;
    glob_t res;

    if (!is_parent) {
        cout << "FINDING MATCHING META FILES..." << endl;
        // add - to end to avoid collision w/ rand section
        hashname = "*." + Util::crypto_hash(path) + "-";
        pattern = path_to_disk_ + hashname + "*";
    } else if (is_parent && path.size() == 1) {
        path = "ROOT";

        hashname = Util::crypto_hash(path);
        pattern  = path_to_disk_ + hashname + ".*";
    } else {
        cout << "Nothing to do..." << endl;
        return meta_files;
    }
    glob(pattern.c_str(), GLOB_NOSORT, NULL, &res);
    cout << "SIZE: " << to_string(res.gl_pathc) << endl;
    if (res.gl_pathc == 0)
        return meta_files;

    // check thru glob stuff
    for (uint64_t ii = 0; ii < res.gl_pathc; ii++) {
        string fpath = res.gl_pathv[ii];
        if (!strstr(fpath.c_str(), ".data"))
            continue;
        // we have a hit
        meta_files.insert(fpath);
    }

    return meta_files;
}
