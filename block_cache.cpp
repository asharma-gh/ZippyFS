#include "block_cache.h"
#include "util.h"
#include "inode.h"
#include "fuse_ops.h"
#include "bplus_index.h"
#include "bplus_index_loader.h"
#include "includes.h"
using namespace std;

// TODO: testing
BlockCache::BlockCache(string path_to_disk) {
    path_to_disk_ = path_to_disk + "root/";
    loader_ = BPLUSIndexLoader(path_to_disk_);
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
    //ptr->set_id(to_string(t));
    //t++;
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

    BPLUSIndexLoader::disk_inode_info di  = get_latest_inode(path, false);
    if (di.i_mtime == 0) {
        cout << "dne" << endl;
        return -ENOENT;
    }

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
vector<BPLUSIndexLoader::index_entry>
BlockCache::readdir(string path) {
    // map (path, index ent)
    map<string, BPLUSIndexLoader::index_entry> added_names;

    unsigned long long time = Util::get_time();
    // first check the cache, add stuff that's updated (checking for deletions)
    // load_from_disk("/mm");
    /*
    for (auto entry : inode_idx_) {
        cout << "Checking " << entry.first << endl;
        char* dirpath = strdup(entry.first.c_str());
        dirpath = dirname(dirpath);
        cout << "DIRNAME " << dirpath << endl;
        if (strcmp(dirpath, path.c_str()) == 0) {
            cout << "ADDING SHIT TO MAP" << endl;
            struct stat st;
            get_inode_by_path(entry.first)->stat(&st);
            BPLUSIndexLoader::index_entry ent;
            ent.path = entry.first;
            ent.deleted = get_inode_by_path(entry.first)->is_deleted();
            ent.mtime = get_inode_by_path(entry.first)->get_ull_mtime();
            added_names[entry.first] = ent;
        }
        free(dirpath);
    }
    */
    unsigned long long dtime = Util::get_time();

    // check stuff on disk
    auto disk_ents = loader_.get_children(path);

    for (auto disk_ent : disk_ents) {
        if (added_names.find(disk_ent.path) == added_names.end()) {
            added_names[disk_ent.path] = disk_ent;
            continue;
        }
        if (added_names[disk_ent.path].mtime > disk_ent.mtime)
            continue;
        added_names[disk_ent.path] = disk_ent;
    }
    vector<BPLUSIndexLoader::index_entry> ents;

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
    if (in_cache(path) == -1)
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
    cout << "Flushing" << endl;
    if (!has_changed_)
        return -1;

    // preprocess size and number of blocks
    uint64_t block_size = 0;
    uint64_t num_blocks = 0;
    for (auto ent : dirty_block_) {
        for (auto bl : ent.second) {
            block_size += bl.second->get_actual_size();
            num_blocks++;
        }
    }

    // preprocess total siz eof paths
    uint64_t psize = 0;
    for (auto ent : inode_idx_) {
        psize += ent.first.size();
    }

    //DiskIndex flusher;
    BPLUSIndex flusher(inode_idx_.size(), block_size, num_blocks, psize);
    for (auto ent : inode_idx_) {
        cout << "Flushing " << ent.first << endl;
        flusher.add_inode(*get_inode_by_path(ent.first),
                          dirty_block_[ent.second], dirty_block_mtime_[ent.second]);
    }
    //cout << "Checking...: " << Util::crypto_hash("/mm") << endl;
    //assert(flusher.find(Util::crypto_hash("/mm")));
    inode_idx_.clear();
    inode_ptrs_.clear();
    dirty_block_.clear();
    size_ = 0;

    return 0;
}

int
BlockCache::load_from_disk(string path) {
    // testing

    cout << "LOADING " << path << " FROM DISK" << endl;

    std::shared_ptr<Inode> latest_inode;
    BPLUSIndexLoader::disk_inode_info di = get_latest_inode(path, true);
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
        cout << "ADDING BLOCK!!!" << endl;
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

BPLUSIndexLoader::disk_inode_info
BlockCache::get_latest_inode(string path, bool get_data) {
    cout << "Looking for thing with loader: " << path << endl;
    return loader_.find_latest_inode(path, get_data);
}
