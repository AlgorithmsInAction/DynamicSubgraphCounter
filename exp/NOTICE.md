# Experiment third-party notices

The experiment harness downloads reference implementations and graph data.
These materials retain their own copyrights and licenses; this repository's
GPLv3 license does not relicense them. Pinned software revisions and data
download recipes are recorded in the [Dockerfile](Dockerfile) and
[get_data.sh](scripts/get_data.sh). Notices for components used by the main
application are in the [root NOTICE](../NOTICE.md).

## Reference implementations

| Component | License and attribution |
| --- | --- |
| [RapidFlow](https://github.com/shixuansun/RapidFlow) | MIT; copyright 2020 Shixuan Sun. See the pinned [LICENSE](https://github.com/shixuansun/RapidFlow/blob/2ea4f4c35bcee95940a5e6c17a74a2a111afc2c1/LICENSE). |
| [ContinuousSubgraphMatching](https://github.com/RapidsAtHKUST/ContinuousSubgraphMatching) (Graphflow, SymBi, IEDyn, TurboFlux) | MIT; copyright 2022 Rapids @ HKUST. See the pinned [LICENSE](https://github.com/RapidsAtHKUST/ContinuousSubgraphMatching/blob/6118049cca9b10046aed5ddd045e207c71dc53a9/LICENSE). |
| [DNA](https://github.com/BenjaminSchiller/DNA) using the following [fork](https://github.com/AlgorithmsInAction/subgraphcounting_DNA) providing a command-line runner | GPLv3. See the pinned [LICENSE](https://github.com/AlgorithmsInAction/subgraphcounting_DNA/blob/170e4e0551e1ea238cb65bd7a2723f0f7a8a61cd/LICENSE) and [third-party notices](https://github.com/AlgorithmsInAction/subgraphcounting_DNA/blob/170e4e0551e1ea238cb65bd7a2723f0f7a8a61cd/THIRD_PARTY_NOTICES.md) for its bundled libraries and incorporated sources. |

## Graph data

The experiments download real-world and synthetic graphs from the sources below.
For real-world data, we use only graph endpoints and timestamps. We do not attempt to identify users and report only aggregate algorithm-performance results. 

| Type | Source and datasets | Attribution and terms |
| --- | --- | --- |
| Real-world | [KONECT](http://konect.cc/): Wikipedia streams (`simple`, `nl`, `pl`, `it`, `fr`, `de`) | Jérôme Kunegis, “[KONECT: The Koblenz Network Collection](https://doi.org/10.1145/2487788.2488173),” *WWW Companion*, 2013. <br> Julia Preusse et al., “[Structural Dynamics of Knowledge Networks](https://doi.org/10.1609/icwsm.v7i1.14402),” *ICWSM*, 2013. |
| Real-world | [SNAP](https://snap.stanford.edu/data/): `sx-askubuntu`, `sx-superuser`, `sx-stackoverflow`, `sx-mathoverflow`, `wiki-talk-temporal` | Ashwin Paranjape, Austin R. Benson, and Jure Leskovec, “[Motifs in Temporal Networks](https://doi.org/10.1145/3018661.3018731),” *WSDM*, 2017. <br> For `wiki-talk-temporal` additionally: Jure Leskovec, Daniel P. Huttenlocher, and Jon M. Kleinberg, “[Governance in Social Media: A Case Study of the Wikipedia Promotion Process](https://doi.org/10.1609/icwsm.v4i1.14013),” *ICWSM*, 2010. <br> See the [SNAP license page](https://snap.stanford.edu/snap/license.html). |
| Real-world | [SNAP](https://snap.stanford.edu/data/): `soc-redditHyperlinks-body`, `soc-redditHyperlinks-title` | Srijan Kumar et al., “[Community Interaction and Conflict on the Web](https://doi.org/10.1145/3178876.3186141),” *WWW*, 2018. <br> See the [SNAP license page](https://snap.stanford.edu/snap/license.html). |
| Real-world | [Network Repository](https://networkrepository.com/): `ia-enron-email-dynamic`, `ia-reality-call`, `soc-wiki-elec`, `tech-as-topology`, `soc-youtube-growth` | Ryan A. Rossi and Nesreen K. Ahmed, “[The Network Data Repository with Interactive Graph Analytics and Visualization](https://doi.org/10.1609/aaai.v29i1.9277),” *AAAI*, 2015. <br> See the [data policy](https://networkrepository.com/policy.php). |
| Synthetic | [DyReach](https://dyreach.taa.univie.ac.at/): Kronecker graphs generated with [SNAP](https://snap.stanford.edu/snap/index.html)'s `krongen` tool | Initiator matrices: Jure Leskovec, Deepayan Chakrabarti, Jon Kleinberg, Christos Faloutsos, and Zoubin Ghahramani, “[Kronecker Graphs: An Approach to Modeling Networks](https://dl.acm.org/doi/10.5555/1756006.1756039),” *Journal of Machine Learning Research* 11 (2010), pp. 985–1042. <br> Data source: Kathrin Hanauer, Monika Henzinger, and Christian Schulz, “[Fully Dynamic Single-Source Reachability in Practice: An Experimental Study](https://doi.org/10.1137/1.9781611976007.9),” *ALENEX 2020*, SIAM, pp. 106–119. |

Prepared streams may undergo undirected conversion, a 14-day edge lifetime
for timestamped inputs, cleaning, truncation, and format conversion, as
described in the [experiment README](README.md).

## Container packages

Ubuntu packages used by the container—including compilers, OpenJDK, Boost,
Qt/qmake, Python, NumPy, pandas, Matplotlib, and PyYAML—retain their individual
licenses and notices. Installed package copyright files are available under
`/usr/share/doc/<package>/copyright` inside the image.
