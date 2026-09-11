#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <boost/container_hash/extensions.hpp>
#include <boost/container_hash/hash_fwd.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_map_fwd.hpp>
#include <cstdint>

#define ILV Algora::IncidenceListVertex
#define CAST_ILV(v) static_cast<Algora::IncidenceListVertex *>(v)

using COUNTER_TYPE = uint64_t;

using TripleKeyMap =
    boost::unordered_flat_map<std::tuple<int, int, int>, COUNTER_TYPE,
                              boost::hash<std::tuple<int, int, int>>>;

using DoubleKeyMap =
    boost::unordered_flat_map<std::pair<int, int>, COUNTER_TYPE>;

using SingleKeyMap = boost::unordered_flat_map<int, COUNTER_TYPE>;

using IncOrDecTripleKey = void(TripleKeyMap &, int, int, int, COUNTER_TYPE);

using IncOrDecDoubleKey = void(DoubleKeyMap &, int, int, COUNTER_TYPE);

using IncOrDecSingleKey = void(SingleKeyMap &, int, COUNTER_TYPE);

using CounterUpdate = void(COUNTER_TYPE &, COUNTER_TYPE);

// Custom hash function for a pair of two 32 bit integers by concatenating
// them into a 64 bit integer and applying the standard boost hash function
namespace std {
template <> struct hash<std::pair<int, int>> {
    auto operator()(const std::pair<int, int> &key) const -> size_t {
        boost::hash<size_t> hasher;
        return hasher((size_t(uint32_t(key.first)) << 32) |
                      size_t(uint32_t(key.second)));

        // return (size_t(uint32_t(key.first)) << 32) |
        // size_t(uint32_t(key.second));
    }
};
} // namespace std

/**
 * @brief Increases the count in the given hash map for the vertex pair
 * specified by their IDs. If the count is 0, creates the entry. The vertex
 * IDs in the hash maps are sorted.
 *
 * @param map Hash map in which to change the count
 * @param first first vertex ID of the vertex pair
 * @param second second vertex ID of the vertex pair
 * @param decrement Nr by which to increase the count
 */
inline void increaseOrCreate(DoubleKeyMap &map, int first, int second,
                             COUNTER_TYPE increment) {
    if (increment <= 0)
        return;

    if (first <= second)
        map[{first, second}] += increment;

    else
        map[{second, first}] += increment;
}

/**
 * @brief Increases the count in the given hash map for the vertex triple
 * specified by their IDs. If the count is 0, create the entry. The vertex
 * IDs in the hash maps are sorted.
 *
 * @param map Hash map in which to change the count
 * @param first first vertex ID of the vertex triple
 * @param second second vertex ID of the vertex triple
 * @param third third vertex ID of the vertex triple
 * @param decrement Nr by which to increase the count
 */
inline void increaseOrCreate(TripleKeyMap &map, int first, int second,
                             int third, COUNTER_TYPE increment)

{
    if (increment <= 0)
        return;

    std::array<int, 3> key = {first, second, third};
    std::sort(key.begin(), key.end());

    map[{key[0], key[1], key[2]}] += increment;
}

/**
 * @brief Increases the count in the given hash map for the vertex specified
 * by the IDs. If the count is 0, create the entry.
 *
 * @param map Hash map in which to change the count
 * @param key vertex ID
 * @param decrement Nr by which to increase the count
 */
inline void increaseOrCreate(SingleKeyMap &map, int key,
                             COUNTER_TYPE increment) {
    if (increment <= 0)
        return;

    map[key] += increment;
}

/**
 * @brief Reduces the count in the given hash map for the vertex pair
 * specified by their IDs. If the count would be <= 0, delete the entry. The
 * vertex IDs in the hash maps are sorted.
 *
 * @param map Hash map in which to change the count
 * @param first first vertex ID of the vertex pair
 * @param second second vertex ID of the vertex pair
 * @param decrement Nr by which to decrease the count
 */
inline void reduceOrDelete(DoubleKeyMap &map, int first, int second,
                           COUNTER_TYPE decrement) {
    if (decrement < 1)
        return;

    auto it = map.end();

    if (first <= second)
        it = map.find({first, second});
    else
        it = map.find({second, first});

    if (it == map.end())
        return;
    if (it->second <= decrement) {
        map.erase(it);
    } else
        it->second -= decrement;
}

/**
 * @brief Reduces the count in the given hash map for the vertex triple
 * specified by their IDs. If the count would be <= 0, delete the entry. The
 * vertex IDs in the hash maps are sorted.
 *
 * @param map Hash map in which to change the count
 * @param first first vertex ID of the vertex triple
 * @param second second vertex ID of the vertex triple
 * @param third third vertex ID of the vertex triple
 * @param decrement Nr by which to decrease the count
 */
inline void reduceOrDelete(TripleKeyMap &map, int first, int second, int third,
                           COUNTER_TYPE decrement) {

    if (decrement < 1)
        return;

    std::array<int, 3> key = {first, second, third};
    std::sort(key.begin(), key.end());

    auto it = map.find({key[0], key[1], key[2]});

    if (it == map.end())
        return;
    if (it->second <= decrement) {
        map.erase(it);
    } else
        it->second -= decrement;
}

/**
 * @brief Reduces the count in the given hash map for the vertex specified
 * by the IDs. If the count would be <= 0, delete the entry.
 *
 * @param map Hash map in which to change the count
 * @param key vertex ID
 * @param decrement Nr by which to decrease the count
 */
inline void reduceOrDelete(SingleKeyMap &map, int key, COUNTER_TYPE decrement) {
    if (decrement < 1)
        return;
    auto it = map.find(key);
    if (it == map.end())
        return;
    if (it->second <= decrement) {
        map.erase(it);
    } else
        it->second -= decrement;
}

#endif