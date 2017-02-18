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
    uint64_t num_blocks = Util::ulong_ceil(size + offset, Block::get_logical_size());
    /*** create inode for write if needed ***/
    shared_ptr<Inode> inode;
    inode = get_inode_by_path(path);
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
    (void)path;
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

unordered_set<string>
BlockCache::find_latest_node(string inode_idx, vector<uint64_t> block_idx) {
    (void)inode_idx;
    (void)block_idx;
    unordered_set<string> nodes;

    // iterate thru all roots and heads, log each .node file
    return nodes;
}

int
BlockCache::flush_to_disk() {
    // create path to .head file
    string fname = Util::generate_rand_hex_name();
    string path_to_index = path_to_disk_ + fname + ".index";
    string path_to_node = path_to_disk_ + fname + ".node";
    string path_to_data = path_to_disk_ + fname + ".data";
    string path_to_root = path_to_disk_ + fname + ".root";

    int indexfd = ::open(path_to_index.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int nodefd = ::open(path_to_node.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int datafd = ::open(path_to_data.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    int rootfd = ::open(path_to_root.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);

    // TODO: look for previous latest .index for the current file
    //  - read each .head file, looking for this inode idx
    //  - record each (.node, offset) pair
    //
    //  - add checksums for each files
    //
    // ===FORMAT for .index===
    // [path] [inode idx] [inode mtime]  [block #s] [offset] [size]

    // create entry for .node file
    uint64_t offset_into_data = 0;
    string header_input;
    uint64_t offset_into_node = 0;

    /** the input for the .root file.
     * Every "flush" constructs a new root
     * TODO: have this root contain previous .index / .nodes
     */
    string root_input;

    /**
     * This loop writes to the .node and .data files
     * .head file is written to last.
     */
    for (auto ent : inode_idx_) {
        // get idxs of dirty blocks
        /*
        vector<uint64_t> db_idxs;
        for (auto db_ents : dirty_block_[ent.second])
            db_idxs.push_back(db_ents.first);
        */
        // fetch record
        shared_ptr<Inode> flushed_inode = get_inode_by_path(ent.first);
        string inode_data = flushed_inode->get_flush_record();
        // get offset map
        auto offst_mp = flushed_inode->get_offsets();
        // make .index entry
        string index_entry = ent.first + " " + ent.second + " " + to_string(flushed_inode->get_ull_mtime()) + " [";
        for (auto ent : offst_mp.second) {
            index_entry += " " + to_string(ent.first);
        }
        index_entry += " ] " + to_string(offset_into_node);

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

        // write node entry size to index entry
        index_entry += + " " + to_string(node_ent_size) + "\n";

        // write to .node
        if (pwrite(nodefd, inode_data.c_str(), inode_data.size() * sizeof(char), 0) == -1)
            cout << "ERROR writing to .node ERRNO: " << strerror(errno) << endl;
        if (pwrite(nodefd, table.c_str(), table.size() * sizeof(char), 0) == -1)
            cout << "ERROR writing TABLE to .node ERRNO: " << strerror(errno);
        // write to .data
        flushed_inode->flush_to_fd(datafd);

        // write to .index
        if (pwrite(indexfd, index_entry.c_str(), index_entry.size() * sizeof(char), 0) == -1)
            cout << "ERROR writing to .index ERRNO: " << strerror(errno) << endl;

        // finally, construct input for .root
        // TODO: redo the format. For now:
        // [path] [index name]
        root_input += ent.first + " " + fname + ".index" + "\n";
    }
    // TODO: stuff with the .root files
    // pull prev. root, copy all .head
    // write to root
    if (pwrite(rootfd, root_input.c_str(), root_input.size() * sizeof(char), 0) == -1)
        cout << "ERROR writing to .root ERRNO: " << strerror(errno) << endl;

    close(indexfd);
    close(nodefd);
    close(datafd);
    close(rootfd);
    return 0;
}

int
BlockCache::load_from_disk(string path) {
    (void)path;
    // find latest root with the given file

    // open directory of roots
    DIR* root_dir = opendir(path_to_disk_.c_str());
    if (root_dir == NULL) {
        cout << "ERROR opening root DIR ERRNO: " << strerror(errno) << endl;
        return -1;
    }

    struct dirent* entry;
    const char* entry_name;
    // iterate thru each entry in root
    while ((entry = ::readdir(root_dir)) != NULL) {
        entry_name = entry->d_name;

        // if this file is not a .root, skip it
        if (strlen(entry_name) < 4
                || strcmp(entry_name + (strlen(entry_name) - 4), ".root") != 0)
            continue;

        // we have a .root file
        // check if it contains this path
        vector<string> index_files = find_entry_in_root(entry_name, path);
        if (index_files.size() == 0)
            // then it is not in this root file, skip it
            continue;
        // if it does, collect the index files for it
        //
        // open the index files
        for (string index : index_files) {
            // find entry in index
            string ent = find_entry_in_index(index, path);
            if (ent.size() == 0) {
                cout << "ERROR GETTING ENTRY IN INDEX" << endl;
                return -1;
            }
            // extract offsets
            char offset_list[ent.size()];
            unsigned long long offset_into_node, node_ent_size, ent_mtime = 0;
            sscanf(ent.c_str(), "%*s %*s %llu %[^]] %llu %llu", &ent_mtime, offset_list, &offset_into_node, &node_ent_size);
            cout << "EXTRACTED " << offset_list << " FROM THE ENT" << endl;
            // find the .node
            // open the .node, offset into it
            // make inode for it
            //
            // open .data
            //
            // load blocks to memory
        }
    }
    return 0;

}

vector<string>
BlockCache::find_entry_in_root(string root_name, string path) {
    vector<string> index_files;
    (void)path;
    string ab_root_path = path_to_disk_ + root_name;
    // iterate thru each entry in this root file
    ifstream in_file(ab_root_path);
    string curline;
    while (getline(in_file, curline)) {
        char cur_path[PATH_MAX];
        char index_ent[FILENAME_MAX];
        sscanf(curline.c_str(), "%s %s", cur_path, index_ent);
        if (strcmp(cur_path, path.c_str()) == 0)
            index_files.push_back((string)index_ent);
    }
    // if we find an entry with this path
    // add all headers to vector
    // return vector
    return index_files;
}

string
BlockCache::find_entry_in_index(string index_name, string path) {
    string path_to_index = path_to_disk_ + index_name;
    ifstream in_file(path_to_index);
    string curline;
    while (getline(in_file, curline)) {
        char cur_path[PATH_MAX];
        sscanf(curline.c_str(), "%s %*s", cur_path);
        if (strcmp(cur_path, path.c_str()) == 0)
            return curline;

    }
    return "";
}
