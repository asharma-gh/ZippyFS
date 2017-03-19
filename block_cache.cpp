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
    if (in_cache(path) == 0)
        return get_inode_by_path(path)->stat(st);

    disk_inode_info di  = get_latest_inode(path, false);
    if (di.i_mtime == 0)
        return -ENOENT;

    st->st_mode = di.i_mode;
    st->st_nlink = di.i_nlink;
    st->st_blocks = di.i_size / Block::get_physical_size() + (di.i_size % Block::get_physical_size() == 0);
    st->st_size = di.i_size;
    st->st_ctim = Util::get_time_ts(di.i_ctime);
    st->st_mtim = Util::get_time_ts(di.i_mtime);

    return 0;

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
    // load_from_disk("/mm");
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
        }
        free(dirpath);
    }
    unsigned long long dtime = Util::get_time();

    // check stuff on disk
    auto disk_ents = get_all_meta_files(path, true);

    for (auto disk_ent : disk_ents) {
        // read path from disk_ent
        ifstream df(disk_ent);
        string ent_line;
        getline(df, ent_line);
        char ent_path[PATH_MAX] = {'\0'};
        sscanf(ent_line.c_str(), "INODE: %s ", ent_path);
        cout << "ENTRY PATH " << ent_path << endl;
        char* dirpath = strdup(ent_path);
        dirpath = dirname(dirpath);
        if (strcmp(dirpath, path.c_str()) == 0) {
            free(dirpath);
            // find latest thing, add entry
            disk_inode_info latest = get_latest_inode((string)ent_path, false);
            if ((added_names.find(ent_path) != added_names.end()
                    && added_names[ent_path].added_time < latest.i_mtime) || added_names.find(ent_path) == added_names.end()) {
                // we have a later entry, update!
                cout << "UPDATING ENTRY IN READDIR" << endl;
                index_entry up_ie;
                up_ie.path = latest.i_path;
                up_ie.added_time = latest.i_mtime;
                up_ie.deleted = latest.i_deleted;
                added_names[ent_path] = up_ie;
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
    cout << "Write " << to_string(size) << " ofst " << to_string(offset) << endl;
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
            dirty_block_mtime_[inode_idx_[path]][block_idx] = inode->get_ull_mtime();

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
    if (in_cache(path) == 0)
        return 0;

    load_from_disk(path);

    // todo make file if it doesnt exist


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
    DiskIndex flusher;
    for (auto ent : inode_idx_) {
        flusher.add_inode(*get_inode_by_path(ent.first), dirty_block_[ent.second]);
    }
    return 0;

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
        if (flushed_inode->is_deleted() == 0) {
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
                              + to_string(block_sz) + " "
                              + to_string(dirty_block_mtime_[inode_idx][blck.first]) + "\n";
                cur_offset += block_sz;
            }
            root_input += data_table;
        }

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
    disk_inode_info di = get_latest_inode(path, true);
    if (di.i_mtime == 0)
        // none exists
        return -1;

    latest_inode = shared_ptr<Inode>(new Inode(path));
    latest_inode->set_id(di.i_inode_id);
    latest_inode->set_size(di.i_size);
    latest_inode->set_mode(di.i_mode);
    latest_inode->set_nlink(di.i_nlink);
    latest_inode->set_mtime(di.i_mtime);
    latest_inode->set_ctime(di.i_ctime);
    if (di.i_deleted)
        latest_inode->delete_inode();

    // add all blocks
    for (auto blk : di.i_block_data) {
        latest_inode->add_block(blk.first, blk.second);
    }

    // add to cache if this one is a later version than the current one, if there is a current one
    bool is_updated = (in_cache(path) == 0) && get_inode_by_path(path)->get_ull_mtime() >= di.i_mtime;

    if (!is_updated || in_cache(path) == -1) {
        cout <<" UPDATED THING " << endl;
        inode_idx_[path] = latest_inode->get_id();
        inode_ptrs_[latest_inode->get_id()] = latest_inode;
        latest_inode->undo_dirty();
    }


    return 0;
}

unordered_set<string>
BlockCache::get_all_meta_files(string path, bool is_parent) {
    (void)path;
    (void)is_parent;
    unordered_set<string> meta_files;
    string hashname;
    string pattern;
    glob_t res;
    cout << "GETTING ALL META FILES FOR " << path << " IS PARENT? " << to_string(is_parent) << endl;
    if (!is_parent) {
        // cout << "FINDING MATCHING META FILES..." << endl;
        // add - to end to avoid collision w/ rand section
        hashname = "*." + Util::crypto_hash(path) + "-";
        pattern = path_to_disk_ + hashname + "*";
    } else {
        if (path.compare("/") == 0)
            path = "ROOT";

        hashname = Util::crypto_hash(path);
        pattern  = path_to_disk_ + hashname + ".*";
    }
    glob(pattern.c_str(), GLOB_NOSORT, NULL, &res);
    cout << "SIZE: " << to_string(res.gl_pathc) << endl;
    if (res.gl_pathc == 0)
        return meta_files;

    // check thru glob stuff
    for (uint64_t ii = 0; ii < res.gl_pathc; ii++) {
        string fpath = res.gl_pathv[ii];
        if (strstr(fpath.c_str(), ".data"))
            continue;
        // we have a hit
        meta_files.insert(fpath);
    }

    return meta_files;
}

BlockCache::disk_inode_info
BlockCache::get_latest_inode(string path, bool get_data) {
    cout << "GETTING LATEST INODE FOR " << path << endl;
    auto meta_files = get_all_meta_files(path, false);
    disk_inode_info cur_inode;
    cur_inode.i_mtime = 0;

    for (auto meta : meta_files) {
        ifstream ifs(meta);
        string curline;
        // update cur_inode only if the mtime is greater
        unsigned long long cur_mtime = 0;
        // first line is always inode info
        getline(ifs, curline);
        char ent_path[PATH_MAX] = {'\0'};
        char ent_id[2048] = {'\0'};
        uint32_t ent_mode = 0;
        uint32_t ent_nlink = 0;
        unsigned long long ent_mtime = 0;
        unsigned long long ent_ctime = 0;
        uint64_t ent_size = 0;
        int ent_deleted = 0;

        sscanf(curline.c_str(),
               "INODE: %s %s %" SCNd32 " %" SCNd32 " %llu %llu %" SCNd64 " %d",
               ent_path, ent_id, &ent_mode, &ent_nlink,
               &ent_mtime, &ent_ctime, &ent_size, &ent_deleted);
        if (cur_mtime <= ent_mtime) {
            // we have an updated entry
            cur_inode.i_path = ent_path;
            cur_inode.i_inode_id = ent_id;
            cur_inode.i_mode = ent_mode;
            cur_inode.i_nlink = ent_nlink;
            cur_inode.i_mtime = ent_mtime;
            cur_inode.i_ctime = ent_ctime;
            cur_inode.i_size = ent_size;
            cur_inode.i_deleted = ent_deleted;
        }
        // if we need the data too, then build up blocks
        // need to add block time to blocks
        if (get_data && !S_ISDIR(cur_inode.i_mode)) {
            // make data file name
            string dpath = meta.substr(0, meta.size() - 5) + ".data";

            // open data file
            int dfd = ::open(dpath.c_str(), O_RDONLY);
            // copy over entries

            while (getline(ifs, curline)) {
                // retrieve stuff
                uint64_t blckidx = 0;
                uint64_t blckoffset = 0;
                uint64_t blcksize = 0;
                unsigned long long blckmtime = 0;

                sscanf(curline.c_str(), "%" SCNd64" %" SCNd64" %" SCNd64" %llu",
                       &blckidx, &blckoffset, &blcksize, &blckmtime);
                // if this block is not updated, then skip it
                if(cur_inode.i_block_time.find(blckidx) != cur_inode.i_block_time.end()
                        && cur_inode.i_block_time[blckidx] > blckmtime)
                    continue;

                // clear to add it, extract data
                uint8_t blckcontent[blcksize] = {'\0'};

                if (pread(dfd, blckcontent, blcksize, blckoffset) == -1)
                    cout << "Error reading data ERRNO: " << strerror(errno) << endl;
                cur_inode.i_block_time[blckidx] = blckmtime;
                cur_inode.i_block_data[blckidx] = shared_ptr<Block>(new Block(blckcontent, blcksize));

            }
            close(dfd);
        }
    }

    return cur_inode;
}
