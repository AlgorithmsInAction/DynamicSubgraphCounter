# SubgraphCounter

SubgraphCounter maintains counts of triangles and connected four-vertex
subgraphs under edge updates. It provides HHH and EGST dynamic algorithms,
plus optional adapters for OB and ESCAPE static baselines.

## Quick start

Requires a C++17 compiler, qmake 5, Boost headers, Git, and unzip. Install the
build dependencies, for example:

**Debian/Ubuntu**

```bash
sudo apt install build-essential qt5-qmake libboost-dev git unzip
```

**Fedora**

```bash
sudo dnf install gcc-c++ make qt5-qtbase-devel boost-devel git unzip
```

From the repository root:

```bash
./easyCompile
./build/Release/SubgraphCounter -i tests/test.txt -a hhh -p epstab -e 0.2 --all
```

The default build fetches OB and ESCAPE, so it needs network access. To build
without those baselines, run `./easyCompile --without-reference-code`; `-a ob`
and `-a escape` are then unavailable. For direct qmake builds, use
`REFERENCE_CODE=OFF` (default: `ON`).

## Source dependencies

[`deps/algora-source.zip`](deps/algora-source.zip) contains the supplied Algora
sources, their GPL-3.0 licenses, and Git metadata used by the upstream build
scripts. `easyCompile` unpacks and builds them in `deps/` if their Debug or
Release libraries are missing.

| Bundled library | Version |
| --- | --- |
| [AlgoraCore](https://gitlab.com/libalgora/AlgoraCore) | [`v1.4`](https://gitlab.com/libalgora/AlgoraCore/-/tags/v1.4) |
| [AlgoraDyn](https://gitlab.com/libalgora/AlgoraDyn) | [`v1.2`](https://gitlab.com/libalgora/AlgoraDyn/-/tags/v1.2) |

The optional static baselines are fetched into `deps/` during the default
build. They are compiled into SubgraphCounter and are not needed at runtime.

| Baseline | Upstream source | Revision |
| --- | --- | --- |
| OB (`-a ob`) | [oaqc](https://github.com/schochastics/oaqc) | [`v2.0.0`](https://github.com/schochastics/oaqc/tree/v2.0.0) |
| ESCAPE (`-a escape`) | [ESCAPE](https://bitbucket.org/seshadhri/escape) | `7ec2f93c524a0b47cdc8c639602e90d1c1fceff3` + [memory-leak patch](src/escape/memory-leaks.patch) |

Build outputs are in `build/Debug` and `build/Release`. If qmake is not on your
`PATH`, pass `--qmake /path/to/qmake`. Use `./easyCompile --clean` to rebuild the
application. A failed OB or ESCAPE download requires network access; use
`--without-reference-code` if those baselines are not needed.

## Running

| Option | Purpose |
| --- | --- |
| `-i FILE` | Dynamic graph input (required). |
| `-a hhh`, `-a egst` | Dynamic algorithms [1, 2]. |
| `-a ob`, `-a escape` | Static baselines [4, 5], when included in the build. |
| `-p epstab -e VALUE` | Epsilon-table partition with the chosen epsilon [1]. |
| `-p hindex` | h-index partition [3]; tune with `--gradual_factor`. |
| `--all` | Count all supported patterns. |
| `--triangle`, `--tPath`, `--claw`, `--paw`, `--fCycle`, `--diamond`, `--fClique` | Count selected patterns. |

HHH uses only the auxiliary structures required for the selected patterns by
default. `--highAnchorsOnly` restricts auxiliary anchors to high-degree
vertices; `--extraAux` enables additional structures; `--no_aux_for_t`
disables triangle auxiliary structures. EGST supports `--direct`. Run
`./build/Release/SubgraphCounter --help` for all options.

For experiment reproduction, see the [experiment guide](exp/README.md).

## References

[1] K. Hanauer, M. Henzinger, and Q. C. Hua, “Fully Dynamic Four-Vertex Subgraph Counting,” in 1st Symposium on Algorithmic Foundations of Dynamic Networks, SAND 2022, March 28-30, 2022, Virtual Conference, J. Aspnes and O. Michail, Eds., in LIPIcs, vol. 221. Schloss Dagstuhl - Leibniz-Zentrum für Informatik, 2022, p. 18:1-18:17. doi: 10.4230/LIPICS.SAND.2022.18.

[2] D. Eppstein, M. T. Goodrich, D. Strash, and L. Trott, “Extended dynamic subgraph statistics using h-index parameterized data structures,” Theor. Comput. Sci., vol. 447, pp. 44–52, 2012, doi: 10.1016/J.TCS.2011.11.034.

[3] D. Eppstein and E. S. Spiro, “The h-Index of a Graph and its Application to Dynamic Subgraph Statistics,” J. Graph Algorithms Appl., vol. 16, no. 2, pp. 543–567, 2012, doi: 10.7155/JGAA.00273.

[4] M. Ortmann and U. Brandes, “Efficient orbit-aware triad and quad census in directed and undirected graphs,” Appl. Netw. Sci., vol. 2, p. 13, 2017, doi: 10.1007/S41109-017-0027-2.

[5] A. Pinar, C. Seshadhri, and V. Vishal, “ESCAPE: Efficiently Counting All 5-Vertex Subgraphs,” in Proceedings of the 26th International Conference on World Wide Web, WWW 2017, Perth, Australia, April 3-7, 2017, R. Barrett, R. Cummings, E. Agichtein, and E. Gabrilovich, Eds., ACM, 2017, pp. 1431–1440. doi: 10.1145/3038912.3052597.


## License

See the [GNU GPLv3 license](LICENSE) and [third-party notices](NOTICE.md).

## Contributors

- Kathrin Hanauer
- Sophia Heck
- Monika Henzinger
- Leonhard Sidl
