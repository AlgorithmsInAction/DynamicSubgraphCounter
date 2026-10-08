# SubgraphCounter: Dynamic, configurable subgraph counting for undirected triangle and 4-vertex patterns


SubgraphCounter dynamically maintains counts of small subgraphs (triangles, 3-paths, claws, paws, 4-cycles, diamonds, 4-cliques) under edge updates. Multiple dynamic algorithms and vertex-partitioning strategies are provided, with optional tuning parameters for performance/accuracy tradeoffs.

## Reference implementations

SubgraphCounter includes adapted reference implementations of two static
subgraph-counting algorithms, integrated as dynamized baselines:

- **OB (Ortmann–Brandes)** [4]: based on **oaqc**;
  [upstream repository](https://github.com/schochastics/oaqc). `easyCompile`
  downloads the pinned `v2.0.0` tag and uses those files directly when compiling the
  project’s dynamisation adapter in
  `src/oaqc/`. Select it with `-a ob`.
- **ESCAPE (Pinar, Seshadhri, and Vishal)** [5]:
  [upstream repository](https://bitbucket.org/seshadhri/escape). `easyCompile`
  downloads revision `7ec2f93c524a0b47cdc8c639602e90d1c1fceff3`, applies
  the source-only memory-leak fixes in `src/escape/memory-leaks.patch`, and uses those files when
  compiling the project’s adapter in
  `src/escape/`. Select it with `-a escape`.

The upstream checkouts are build dependencies only: they are not stored in the
repository and are not needed at runtime. `easyCompile` keeps them in the
Git-ignored `deps/` directory. The adapted implementations are compiled into
SubgraphCounter as part of the normal build.

## Building

This application is built on **Algora** and uses implementations based on the **STL** with additional support from selected **Boost libraries**. The build process is managed using `qmake` version 5.

To install the required dependencies, run the following commands:

**Debian/Ubuntu**:
  `bash
  sudo apt install qt5-qmake libboost-dev
  `

**Fedora**:
  `bash
  sudo dnf install qt5-qtbase-devel boost-devel
  `

### Building the Application

The source of both required Algora libraries is included in
[`deps/algora-source.zip`](deps/algora-source.zip). It was
packaged from the supplied [AlgoraCore](https://gitlab.com/libalgora/AlgoraCore)
and [AlgoraDyn](https://gitlab.com/libalgora/AlgoraDyn) source checkouts at
these revisions:

| Library | Version |
| --- | --- | 
| AlgoraCore | [`v1.4`](https://gitlab.com/libalgora/AlgoraCore/-/tags/v1.4) |
| AlgoraDyn | [`v1.2`](https://gitlab.com/libalgora/AlgoraDyn/-/tags/v1.2) |

Both are GPL-3.0 licensed; each source directory includes its `COPYING` and
`LICENSE` files. The zip also retains Git metadata because the Algora
build scripts embed revision information. Running `./easyCompile` unpacks the
source into `deps/AlgoraCore` and `deps/AlgoraDyn`, then compiles both libraries
only if their Debug or Release archive is missing. The application's qmake
project uses those paths. No separate Algora checkout or manual build is needed:

```bash
./easyCompile
```

The compiled libraries are placed in `deps/AlgoraCore/build/` and
`deps/AlgoraDyn/build/`. If you prefer to build them manually, extract the zip
into `deps/`, then run each library's `easyCompile` script there before building
this application with qmake.

Reference implementations (OB and ESCAPE) are included by default. To build
without fetching or compiling them, run `./easyCompile --without-reference-code`.
For a direct qmake build, pass `REFERENCE_CODE=OFF` (the default is `ON`).
In this configuration, `-a ob` and `-a escape` report that they are unavailable.
---


## Running

Example run:

```
$  ./build/Release/SubgraphCounter -i tests/test.txt -a hhh --highAnchorsOnly --extraAux -p epstab --rebalance_mode lazy -e 0.2 --all
```

| **Option**| **Value**|**Explanation**
|--|--|--|
| `-a, --algo`      | `hhh` <br> `egst` <br> `ob` <br> `escape`               | **`hhh`**: Dynamic algorithm based on the algorithm by Hanauer, Henzinger, Hua [1]. Optional parameters: `--highAnchorsOnly` to require all internal auxiliary structures to have high degree anchors and `--extraAux` to maintain and use all possible auxiliary structures for the given patterns. <br><br> **`egst`**: Dynamic algorithm based on the algorithm by Eppstein, Goodrich, Strash, and Trott [2]. Optional parameters: `--highAnchorsOnly` to require all internal auxiliary structures to have high degree anchors and `--direct` to apply a direct routine for vertex partition swaps. <br><br>  **`ob`**: Dynamized version of the [static algorithm from Ortman and Brandes](https://cran.r-project.org/web/packages/oaqc/index.html). [4] <br><br> **`escape`**: Dynamized version of the [static algorithm from  Pinar, Seshadhri, and Vishal](https://bitbucket.org/seshadhri/escape) [5]. |
| `-p, --partition` | `epstab` <br> `hindex`         | **`epstab`**: Uses the Epsilon Table from [1] to partition vertices based on their degree (high or low). The epsilon value can be set using the `-e, --epsilon` parameter (required). Optionally, the rebalance factor can be set with the `-r,--rebalance_factor` parameter (default r=2) and are three rebalance modes (`soft, lazy, late`) different from the original one that can be set with the `"-m,--rebalance_mode` parameter. <br><br> **`hindex`**: Uses the h-Index from [3] to partition vertices based on their degree (high or low). Optionally, the gradual factor can be set using the `--gradual_factor` parameter (default gradual_factor=2).  |
|`--all`  ||  Count all subgraphs simultaneously |
|`--triangle` | |   Count triangles |
|`--tPath` ||   Count three paths |
|`--claw` ||   Count claws |
|`--paw`"  ||  Count paws |
|`--fCycle` | |   Count four cycles |
|`--diamond` ||   Count diamonds |
|`--fClique` ||   Count four cliques |

HHH maintains only the auxiliary structures required for the selected patterns
by default. Pass `--extraAux` explicitly to enable additional auxiliary
structures. `--no_aux_for_t` independently disables triangle auxiliary
structures.


For further configuration options see `./build/Release/SubgraphCounter --help`. 


[1] K. Hanauer, M. Henzinger, and Q. C. Hua, “Fully Dynamic Four-Vertex Subgraph Counting,” in 1st Symposium on Algorithmic Foundations of Dynamic Networks, SAND 2022, March 28-30, 2022, Virtual Conference, J. Aspnes and O. Michail, Eds., in LIPIcs, vol. 221. Schloss Dagstuhl - Leibniz-Zentrum für Informatik, 2022, p. 18:1-18:17. doi: 10.4230/LIPICS.SAND.2022.18.

[2] D. Eppstein, M. T. Goodrich, D. Strash, and L. Trott, “Extended dynamic subgraph statistics using h-index parameterized data structures,” Theor. Comput. Sci., vol. 447, pp. 44–52, 2012, doi: 10.1016/J.TCS.2011.11.034.

[3] D. Eppstein and E. S. Spiro, “The h-Index of a Graph and its Application to Dynamic Subgraph Statistics,” J. Graph Algorithms Appl., vol. 16, no. 2, pp. 543–567, 2012, doi: 10.7155/JGAA.00273.

[4] M. Ortmann and U. Brandes, “Efficient orbit-aware triad and quad census in directed and undirected graphs,” Appl. Netw. Sci., vol. 2, p. 13, 2017, doi: 10.1007/S41109-017-0027-2.

[5] A. Pinar, C. Seshadhri, and V. Vishal, “ESCAPE: Efficiently Counting All 5-Vertex Subgraphs,” in Proceedings of the 26th International Conference on World Wide Web, WWW 2017, Perth, Australia, April 3-7, 2017, R. Barrett, R. Cummings, E. Agichtein, and E. Gabrilovich, Eds., ACM, 2017, pp. 1431–1440. doi: 10.1145/3038912.3052597.


## Contributors

- Kathrin Hanauer
- Sophia Heck
- Monika Henzinger
- Leonhard Sidl
