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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_DATABASE_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_DATABASE_HPP

#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/hash_table/slab_hash_table.hpp>
#include <bitcoin/database/hash_table/slab_manager.hpp>

namespace libbitcoin {
namespace database {

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API transaction_database
{
public:
    transaction_database(const boost::filesystem::path& map_filename);

    /// Initialize a new transaction database.
    void create();

    /// Call before using the database.
    void start();

    /// Call stop to unload the memory map.
    bool stop();

    /// Fetch transaction from its hash.
    transaction_result get(const hash_digest& hash) const;

    /// Store a transaction in the database. Returns a unique index
    /// which can be used to reference the transaction.
    void store(size_t height, size_t index, const chain::transaction& tx);

    /// Delete a transaction from database.
    void remove(const hash_digest& hash);

    /// Synchronise storage with disk so things are consistent.
    /// Should be done at the end of every block write.
    void sync();

private:
    typedef slab_hash_table<hash_digest> slab_map;

    // Hash table used for looking up txs by hash.
    memory_map map_file_;
    slab_hash_table_header header_;
    slab_manager manager_;
    slab_map map_;
};

} // namespace database
} // namespace libbitcoin

#endif