#ifndef GET_ALGORITHM_H
#define GET_ALGORITHM_H

#include <memory>

#include "config.h"
#include "e_counts/ESubgraphCounts.h"
#ifdef WITH_REFERENCE_CODE
#include "escape/DynamizedESCAPE.h"
#include "oaqc/DynamizedOBASubgraphCounts.h"
#endif
#include "h_counts/HSubgraphCounts.h"
#include "h_vanilla/HVSubgraphCounts.h"
#include "partition/EpsilonTab.h"
#include "partition/HIndex.h"
#include "partition/MockPartition.h"
#include "partition/VertexPartition.h"

SubgraphCounts *get_algorithm(Config &config) {

    switch (config.algo) {
    case HHH:
        return new HSubgraphCounts(config);
        break;
    case HHH_VANILLA:
        return new HVSubgraphCounts(config);
        break;
    case EGST:
        return new ESubgraphCounts(config);
        break;
#ifdef WITH_REFERENCE_CODE
    case OB:
        return new DynamizedOBASubgraphCounts(config);
        break;
    case ESCAPE:
        return new DynamizedESCAPE(config);
        break;
#endif
    default:
        exit(0);
        break;
    }
}

VertexPartition *get_partition(Config &config) {

    switch (config.part) {
    case EPS_TAB:
        return new EpsilonTab(config);
        break;
    case HINDEX:
        return new HIndex(config);
        break;
    case MOCK:
        return new MockPartition(config.maintain_part_stats);
        break;
    case NONE:
        return nullptr;
        break;
    default:
        exit(0);
        break;
    }
}

#endif /* end of include guard: GET_ALGORITHM_H */
