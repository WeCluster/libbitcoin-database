/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_RECORD_ROW_IPP
#define LIBBITCOIN_DATABASE_RECORD_ROW_IPP

#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/**
 * Item for record_hash_table. A chained list with the key included.
 *
 * Stores the key, next index and user data.
 * With the starting item, we can iterate until the end using the
 * next_index() method.
 */
template <typename HashType>
class record_row
{
public:
    static BC_CONSTEXPR size_t index_size = sizeof(array_index);
    static BC_CONSTEXPR size_t key_size = std::tuple_size<HashType>::value;
    static BC_CONSTEXPR file_offset value_begin = key_size + index_size;

    record_row(record_manager& manager, array_index index);

    array_index create(const HashType& key, const array_index next);

    /// Does this match?
    bool compare(const HashType& key) const;

    /// The actual user data.
    const memory_ptr data() const;

    /// Position of next item in the chained list.
    array_index next_index() const;

    /// Write a new next index.
    void write_next_index(array_index next);

private:
    const memory_ptr raw_next_data() const;
    const memory_ptr raw_data(file_offset offset) const;

    array_index index_;
    record_manager& manager_;
    mutable shared_mutex mutex_;
};

template <typename HashType>
record_row<HashType>::record_row(record_manager& manager,
    const array_index index)
  : manager_(manager), index_(index)
{
    static_assert(index_size == 4, "Invalid array_index size.");
}

template <typename HashType>
array_index record_row<HashType>::create(const HashType& key,
    const array_index next)
{
    // Create new record.
    //   [ HashType ]
    //   [ next:4   ]
    //   [ value... ]
    index_ = manager_.new_records(1);

    // Write record.
    const auto memory = raw_data(0);
    const auto record = REMAP_ADDRESS(memory);
    auto serial = make_serializer(record);
    serial.write_data(key);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock(mutex_);
    serial.template write_little_endian<array_index>(next);

    return index_;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename HashType>
bool record_row<HashType>::compare(const HashType& key) const
{
    // Key data is at the start.
    const auto memory = raw_data(0);
    return std::equal(key.begin(), key.end(), REMAP_ADDRESS(memory));
}

template <typename HashType>
const memory_ptr record_row<HashType>::data() const
{
    // Value data is at the end.
    return raw_data(value_begin);
}

template <typename HashType>
array_index record_row<HashType>::next_index() const
{
    const auto memory = raw_next_data();
    const auto next_address = REMAP_ADDRESS(memory);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock(mutex_);
    return from_little_endian_unsafe<array_index>(next_address);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename HashType>
void record_row<HashType>::write_next_index(array_index next)
{
    const auto memory = raw_next_data();
    auto serial = make_serializer(REMAP_ADDRESS(memory));

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock(mutex_);
    serial.template write_little_endian<array_index>(next);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename HashType>
const memory_ptr record_row<HashType>::raw_data(file_offset offset) const
{
    auto memory = manager_.get(index_);
    REMAP_INCREMENT(memory, offset);
    return memory;
}

template <typename HashType>
const memory_ptr record_row<HashType>::raw_next_data() const
{
    // Next position is after key data.
    return raw_data(key_size);
}

} // namespace database
} // namespace libbitcoin

#endif
