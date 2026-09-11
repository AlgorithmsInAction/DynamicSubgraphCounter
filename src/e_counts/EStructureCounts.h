#pragma once

#include "StructureCounts.h"
#include "definitions.h"
#include "graph.dyn/dynamicdigraph.h"
#include "graph/arc.h"
#include "graph/vertex.h"
#include "graph/vertexpair.h"
#include "partition/VertexPartition.h"
#include <boost/unordered/unordered_map_fwd.hpp>
#include <functional>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

// #define TRACK_CACHE
// #define TRACK_DEEP_TIME

#ifdef TRACK_CACHE
#include "util/cache_miss.h"
#endif
/**
 * @brief Manages all auxiliary counts.
 *
 */
class EStructureCounts : public StructureCounts {
  public:
    EStructureCounts() = default;
    ~EStructureCounts() override = default;

    SingleKeyMap s0;
    SingleKeyMap s1;
    DoubleKeyMap s2;
    DoubleKeyMap s3;
    DoubleKeyMap s4;
    DoubleKeyMap s5;
    DoubleKeyMap s6;
    TripleKeyMap s7;
    bool count_s[8];

    bool directVertexChange{0};
    bool highAnchorsOnlyS3{0};

#if ENABLE_DEEPER_STATS
    size_t op_s0{0};
    size_t op_s1{0};
    size_t op_s2{0};
    size_t op_s3{0};
    size_t op_s4{0};
    size_t op_s5{0};
    size_t op_s6{0};
    size_t op_s7{0};
#endif

#ifdef TRACK_CACHE
    MemoryCostCounter cacheMisses;
    MemoryCostCounter cacheMissesVertex;
#endif

#ifdef TRACK_DEEP_TIME
    std::chrono::duration<double> outer_updateForV_Time{0};
    std::chrono::duration<double> inner_updateForV_Time{0};
    std::chrono::duration<double> s3_Time{0};
#endif

    friend class SubgraphCounts;

    void VertexChange(const Algora::Vertex *v, IncOrDecSingleKey *operation1,
                      IncOrDecDoubleKey *operation2,
                      IncOrDecTripleKey *operation3);
    void ArcChange(const Algora::Arc *a, IncOrDecSingleKey *operation1,
                   IncOrDecDoubleKey *operation2,
                   IncOrDecTripleKey *operation3);

    void VertexChange(Algora::Vertex *v, IncOrDecSingleKey *operation1a,
                      IncOrDecSingleKey *operation1b,
                      IncOrDecDoubleKey *operation2a,
                      IncOrDecDoubleKey *operation2b,
                      IncOrDecTripleKey *operation3a,
                      IncOrDecTripleKey *operation3b);

  public:
    void setObjectives(bool s0 = true, bool s1 = true, bool s2 = true,
                       bool s3 = true, bool s4 = true, bool s5 = true,
                       bool s6 = true, bool s7 = true);

    bool initVertexPartition() override;
    bool prepare() override;
    void cleanMaps() override;
    void cleanup() override;

    bool recompute(
        bool arcAdd,
        boost::unordered_map<const Algora::Vertex *, int> &arcBorder) override;

    VertexPartition *setVertexPartition(VertexPartition *partition) {
        return vPart = partition;
    }

    /**
     * @brief Get the number of s0 structures for the given anchor vertices.
     *
     * @param v anchor vertex
     * @return int
     */
    int getNrs0(const Algora::Vertex *v) {
        auto it = s0.find(v->getId());
        return (it == s0.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s0 and their counts.
     *
     * @return auto
     */
    auto gets0begin() const { return s0.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s0 and their counts.
     *
     * @return auto
     */
    auto gets0end() const { return s0.end(); }

    /**
     * @brief Get the number of s1 structures for the given anchor vertices.
     *
     * @param v anchor vertex
     * @return int
     */
    int getNrs1(const Algora::Vertex *v) {
        auto it = s1.find(v->getId());
        return (it == s1.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s1 and their counts.
     *
     * @return auto
     */
    auto gets1begin() const { return s1.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s1 and their counts.
     *
     * @return auto
     */
    auto gets1end() const { return s1.end(); }

    /**
     * @brief Get the number of s2 structures for the given anchor vertices.
     * The anchor vertices can be given in any order.
     *
     * @param v1 first anchor vertex
     * @param v2 second anchor vertex
     * @return int
     */
    int getNrs2(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = s2.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = s2.find({id1, id2});
        else
            it = s2.find({id2, id1});

        return (it == s2.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s2 and their counts.
     *
     * @return auto
     */
    auto gets2begin() const { return s2.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s2 and their counts.
     *
     * @return auto
     */
    auto gets2end() const { return s2.end(); }

    /**
     * @brief Get the number of s3 structures for the given anchor vertices.
     * The anchor vertices can be given in any order.
     *
     * @param v1 first anchor vertex
     * @param v2 second anchor vertex
     * @return int
     */
    int getNrs3(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = s3.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = s3.find({id1, id2});
        else
            it = s3.find({id2, id1});

        return (it == s3.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s3 and their counts.
     *
     * @return auto
     */
    auto gets3begin() const { return s3.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s3 and their counts.
     *
     * @return auto
     */
    auto gets3end() const { return s3.end(); }

    /**
     * @brief Get the number of s4 structures for the given anchor vertices.
     * The anchor vertices can be given in any order.
     *
     * @param v1 first anchor vertex
     * @param v2 second anchor vertex
     * @return int
     */
    int getNrs4(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = s4.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = s4.find({id1, id2});
        else
            it = s4.find({id2, id1});

        return (it == s4.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s4 and their counts.
     *
     * @return auto
     */
    auto gets4begin() const { return s4.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s4 and their counts.
     *
     * @return auto
     */
    auto gets4end() const { return s4.end(); }

    /**
     * @brief Get the number of s5 structures for the given anchor vertices.
     * The anchor vertices can be given in any order.
     *
     * @param v1 first anchor vertex
     * @param v2 second anchor vertex
     * @return int
     */
    int getNrs5(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = s5.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = s5.find({id1, id2});
        else
            it = s5.find({id2, id1});

        return (it == s5.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s5 and their counts.
     *
     * @return auto
     */
    auto gets5begin() const { return s5.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s5 and their counts.
     *
     * @return auto
     */
    auto gets5end() const { return s5.end(); }

    /**
     * @brief Get the number of s6 structures for the given anchor vertices.
     * The anchor vertices can be given in any order.
     *
     * @param v1 first anchor vertex
     * @param v2 second anchor vertex
     * @return int
     */
    int getNrs6(const Algora::Vertex *v1, const Algora::Vertex *v2) {
        auto it = s6.end();
        int id1 = v1->getId();
        int id2 = v2->getId();

        if (id1 < id2)
            it = s6.find({id1, id2});
        else
            it = s6.find({id2, id1});

        return (it == s6.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s6 and their counts.
     *
     * @return auto
     */
    auto gets6begin() const { return s6.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s6 and their counts.
     *
     * @return auto
     */
    auto gets6end() const { return s6.end(); }

    /**
     * @brief Get the number of s7 structures for the given anchor vertices.
     * The anchor vertices can be given in any order.
     *
     * @param v1 first anchor vertex
     * @param v2 second anchor vertex
     * @param v3 third anchor vertex
     * @return int
     */
    int getNrs7(const Algora::Vertex *v1, const Algora::Vertex *v2,
                const Algora::Vertex *v3) {
        auto it = s7.end();
        int id1 = v1->getId();
        int id2 = v2->getId();
        int id3 = v3->getId();

        if (id1 <= id2 && id2 <= id3)
            it = s7.find({id1, id2, id3});
        else if (id1 <= id3 && id3 <= id2)
            it = s7.find({id1, id3, id2});
        else if (id2 <= id1 && id1 <= id3)
            it = s7.find({id2, id1, id3});
        else if (id2 <= id3 && id3 <= id1)
            it = s7.find({id2, id3, id1});
        else if (id3 <= id1 && id1 <= id2)
            it = s7.find({id3, id1, id2});
        else if (id3 <= id2 && id2 <= id1)
            it = s7.find({id3, id2, id1});

        return (it == s7.end() ? 0 : it->second);
    }
    /**
     * @brief Get begin-iterator of the hahs map containing all vertices which
     * are anchor vertices of the the auxiliary structure s7 and their counts.
     *
     * @return auto
     */
    auto gets7begin() const { return s7.begin(); }
    /**
     * @brief Get end-iterator of the hahs map containing all vertices which are
     * anchor vertices of the the auxiliary structure s7 and their counts.
     *
     * @return auto
     */
    auto gets7end() const { return s7.end(); }

    void printEpsTable() { vPart->print_table(); }

    void setSoftVertexChange(bool soft) { directVertexChange = soft; }
    void setHighAnchorsOnlyS3(bool high_only) { highAnchorsOnlyS3 = high_only; }
};
