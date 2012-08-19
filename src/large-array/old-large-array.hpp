/**
 By Johannes Goller, created on 2009-05-12
 @author $Author: goller $
 @version $Revision: 1366 $
 @lastrevision $Date: 2010-05-07 20:16:23 +0900 (金, 07 5 2010) $

 Very large arrays, organized in blocks.
 */

#ifndef _LARGE_ARRAY_H_
#define _LARGE_ARRAY_H_

#include <vector>
#include <boost/pool/pool.hpp>

namespace sufex {

/** Wrapper around malloc & Co. The idea is that a similar wrapper can be
 * created for memory management relying on secondary storage. */
template <class T>
class heap_mem_t {
public:
  /** Address type. */
  typedef T* loc_t;

  /** Loading an address into RAM. */
  T* access(const loc_t loc) { return loc; }
  const T* const_access(const loc_t loc) { return const_cast<const T*>(loc); }

  /** Leaving a space, possibly causing it to be unloaded from RAM. */
  void leave(const loc_t /*loc*/) {}

  /** Allocating space. */
  loc_t alloc(const bit32_t num_units) { return reinterpret_cast<loc_t>(malloc(sizeof(T)*num_units)); }
  /** Reallocating space. */
  loc_t realloc(const loc_t   loc,
                const bit32_t num_units) {
    return reinterpret_cast<loc_t>(realloc(loc,sizeof(T)*num_units));
  }
  /** Deallocating space. */
  void dealloc(const loc_t   loc,
               const bit32_t /*num_units*/) { free(loc); }
};

template <class int_T1, class int_T2>
inline int_T1 int_div(const int_T1 dividend, const int_T2 divisor) {
  return (dividend / divisor) + ((dividend % divisor) != 0?1:0);
}

template <class T, class mem_T = heap_mem_t<T> >
class large_array_t {
public:
  typedef std::pair<typename mem_T::loc_t,bit64_t> dir_entry_t;
  typedef std::vector<dir_entry_t>                 directory_t;

  large_array_t(mem_T         *const mem,
                const bit32_t  units_per_block = 10000000ul,
                logger_t      *const logger = 0)
  :_logger(logger),_mem(mem),_units_per_block(units_per_block),_directory(),_total_size(0)
  {}

  large_array_t(const large_array_t &LA);
  large_array_t<T,mem_T> &operator=(const large_array_t<T,mem_T> &LA);

  /** The destructor does not deallocate the array. This is useful if the
   * mem_T stores on secondary memory. */
  ~large_array_t() {}

  /** Deallocate and empty the entire array. */
  void dealloc();

  /** Change the size. This may cause deallocations, allocations and reallocations
   * of blocks as necessary. */
  void set_size(const bit64_t new_size);

  bit32_t get_units_per_block() const {
    return this->_units_per_block;
  }

  class iterator;
  iterator begin() {
    return iterator(this->_mem,
        this->_directory.begin(),this->_directory.end(),0,this->_units_per_block);
  }

  T get_val(const bit64_t pos) const {
    const bit32_t block_index = pos / this->_units_per_block;
    assert(block_index < this->_directory.size());
    T *const block_begin = this->_mem->access(this->_directory[block_index].first);
    const bit32_t in_block_pos = static_cast<bit32_t>(pos % this->_units_per_block);
    assert(in_block_pos < this->_directory[block_index].second);
    const T cur_val = block_begin[in_block_pos];
    this->_mem->leave(this->_directory[block_index].first);
    return cur_val;
  }

  void set_val(const bit64_t pos,
               const T new_val) {
    const bit32_t block_index = pos / this->_units_per_block;
    assert(block_index < this->_directory.size());
    T *const block_begin = this->_mem->access(this->_directory[block_index].first);
    const bit32_t in_block_pos = static_cast<bit32_t>(pos % this->_units_per_block);
    assert(in_block_pos < this->_directory[block_index].second);
    block_begin[in_block_pos] = new_val;
    this->_mem->leave(this->_directory[block_index].first);
  }

  void add_to_val(const bit64_t pos,
                  const T new_val) {
    const bit32_t block_index = pos / this->_units_per_block;
    assert(block_index < this->_directory.size());
    T *const block_begin = this->_mem->access(this->_directory[block_index].first);
    const bit32_t in_block_pos = static_cast<bit32_t>(pos % this->_units_per_block);
    assert(in_block_pos < this->_directory[block_index].second);
    block_begin[in_block_pos] += new_val;
    this->_mem->leave(this->_directory[block_index].first);
  }

  /** Multi-threaded implementation of an add-operation, i.e. adding all
   * elements of src_LA to our own.
   * ASSUMPTION: Both arrays are identical in length. [Otherwise an exception
   * will be thrown if in assert-mode.] */
  void add(large_array_t<T,mem_T> &src_LA,
           bit16_t max_threads = 1);

  /** Set all elements to the value specified. */
  void set_all(const T val);

private:
  /** Logging and memory management. */
  logger_t      *_logger;
  mem_T         *_mem;
  /** Block size. */
  bit32_t        _units_per_block;
  /** Block directory. */
  directory_t    _directory;
  /** Total number of elements. */
  bit64_t        _total_size;
};

template <class T, class mem_T>
class large_array_t<T,mem_T>::iterator {
public:
  iterator(mem_T *const mem,
           const typename directory_t::iterator block_it,
           const typename directory_t::iterator block_end_it,
           const bit32_t in_block_pos,
           const bit32_t units_per_block)
  :_mem(mem),_block_it(block_it),_block_end_it(block_end_it),
  _block_begin(0),
  _in_block_pos(in_block_pos),_units_per_block(units_per_block),
  _eoi(false) {
    if (block_it != block_end_it)
      this->_access_block();
    else
      this->_eoi = true;
  }
  iterator(mem_T *const mem,
           const typename directory_t::iterator block_it,
           const typename directory_t::iterator block_end_it,
           const bit32_t units_per_block)
  :_mem(mem),_block_it(block_it),_block_end_it(block_end_it),
  _block_begin(0),
  _in_block_pos(0),_units_per_block(units_per_block),
  _eoi(false)
  {
    if (block_it != block_end_it)
      this->_access_block();
    else
      this->_eoi = true;
  }

  ~iterator() {
    if (!this->_eoi)
      this->_leave_block();
  }

  bool eoi() const {
    return this->_eoi;
  }

  iterator &operator++() {
    assert(!this->_eoi);
    ++this->_in_block_pos;
    if (this->_in_block_pos >= this->_block_it->second) {
      this->_leave_block();
      this->_in_block_pos = 0;
      ++this->_block_it;
      if (this->_block_it == this->_block_end_it)
        this->_eoi = true;
      else
        this->_access_block();
    }
    return *this;
  }

  iterator &operator+=(const bit64_t x) {
    assert(!this->_eoi);
    bit64_t new_pos = static_cast<bit64_t>(this->_in_block_pos) + x;
    if (new_pos >= static_cast<bit64_t>(this->_block_it->second)) {
      this->_leave_block();
      do {
        new_pos -= this->_block_it->second;
        ++this->_block_it;
        if (this->_block_it == this->_block_end_it) {
          this->_eoi = true;
          return *this;
        }
      } while (new_pos >= this->_block_it->second);
      this->_access_block();
    }
    this->_in_block_pos = static_cast<bit32_t>(new_pos);
    return *this;
  }

  T &operator*() {
    assert(!this->_eoi);
    return this->_block_begin[this->_in_block_pos];
  }

protected:
  void _access_block() {
    this->_block_begin = this->_mem->access(this->_block_it->first);
  }
  void _leave_block() {
    this->_mem->leave(this->_block_it->first);
  }

private:
  mem_T *_mem;
  typename directory_t::iterator  _block_it;
  typename directory_t::iterator  _block_end_it;
  T             *_block_begin;
  bit32_t        _in_block_pos;
  const bit32_t  _units_per_block;
  bool           _eoi;
};

/** Thread implementation that can be used to add the elements
 * of one large array to those of another. The thread is responsible
 * for a certain range of the large array, to be defined at construction
 * time by the caller. */
template <class T, class mem_T = heap_mem_t<T> >
class large_array_plus_thread_t : public ito_t {
public:
  large_array_plus_thread_t(const typename large_array_t<T,mem_T>::iterator src_it,
                            const typename large_array_t<T,mem_T>::iterator dest_it,
                            const bit64_t range,
                            logger_t *const logger = 0)
  :ito_t(logger),_src_it(src_it),_dest_it(dest_it),_max(range),_pos(0) {}

  virtual void *run() {
    while (this->_pos < this->_max) {
      assert(!this->_src_it.eoi());
      assert(!this->_dest_it.eoi());
      *this->_dest_it += *this->_src_it;
      ++this->_pos;
      ++this->_src_it;
      ++this->_dest_it;
    }
    return 0;
  }

private:
  /** Current position in source and dest array. */
  typename large_array_t<T,mem_T>::iterator _src_it;
  typename large_array_t<T,mem_T>::iterator _dest_it;
  /** Length of the range. */
  const bit64_t   _max;
  /** Positions processed. */
  bit64_t         _pos;
};

} // end of namespace

#endif
