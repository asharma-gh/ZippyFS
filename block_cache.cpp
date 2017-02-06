#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include "includes.h"
using namespace std;
// TODO: get rid of shadow directory
BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {}

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
    // new!
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

    // new!
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

    // new!
    inode_idx_[to] = ll->get_id();
    inode_ptrs_[ll->get_id()] = ll;

    return 0;
}

// new!
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
    // grab blocks
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

// new!
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

        } else {
            shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));
            // add newly formed block to the inode
            inode->add_block(block_idx, ptr);
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
    //new!
    return get_inode_by_path(path)->read(buf, size, offset);
}

int
BlockCache::truncate(string path, uint64_t size) {
    if (in_cache(path) == -1)
        return -1;

    //new!
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

    // iterate thru normalized maps
    //
    // flush data into files in some format
    //
    // close files
    return 0;
}

int
BlockCache::load_from_disk() {

    return 0;
}
