# Experiment third-party notices

Downloaded software and data retain their original licenses and copyrights;
the repository's GPLv3 does not relicense them. Revisions and download recipes
are recorded in [Dockerfile](Dockerfile) and [get_data.sh](scripts/get_data.sh).

## Algorithms

| Component | License and notice |
| --- | --- |
| RapidFlow | MIT; copyright 2020 Shixuan Sun; [LICENSE](https://github.com/shixuansun/RapidFlow/blob/2ea4f4c35bcee95940a5e6c17a74a2a111afc2c1/LICENSE). |
| ContinuousSubgraphMatching (Graphflow, SymBi, IEDyn, TurboFlux) | MIT; copyright 2022 Rapids @ HKUST; [LICENSE](https://github.com/RapidsAtHKUST/ContinuousSubgraphMatching/blob/6118049cca9b10046aed5ddd045e207c71dc53a9/LICENSE). |
| DNA | GPLv3; [LICENSE](https://github.com/AlgorithmsInAction/subgraphcounting_DNA/blob/170e4e0551e1ea238cb65bd7a2723f0f7a8a61cd/LICENSE). Bundled libraries and incorporated sources retain separate licenses listed in [THIRD_PARTY_NOTICES.md](https://github.com/AlgorithmsInAction/subgraphcounting_DNA/blob/170e4e0551e1ea238cb65bd7a2723f0f7a8a61cd/THIRD_PARTY_NOTICES.md). |
| OB / oaqc | GPLv3 or later; see the [root notice](../NOTICE.md). |
| ESCAPE | Apache-2.0; see the [root notice](../NOTICE.md). |
| AlgoraCore / AlgoraDyn (bundled, not downloaded) | GPLv3; license texts in `deps/algora-source.zip`; see the [root notice](../NOTICE.md). |

## Data

| Source and downloaded datasets | License / attribution |
| --- | --- |
| [DyReach](https://dyreach.taa.univie.ac.at/): constant-size and growing Kronecker streams; KONECT Wikipedia streams (`simple`, `nl`, `pl`, `it`, `fr`, `de`) | No explicit data license stated on the download page (its GPLv3 statement covers software). Cite Hanauer, Henzinger, and Schulz, *Fully Dynamic Single-Source Reachability in Practice* (ALENEX 2020); retain original KONECT notices for Wikipedia data. |
| [SNAP](https://snap.stanford.edu/data/): `sx-askubuntu`, `sx-superuser`, `sx-stackoverflow`, `sx-mathoverflow`, `wiki-talk-temporal`, `soc-redditHyperlinks-body/title` | No explicit data license stated on the checked dataset pages. Follow original-source terms and dataset citation requests: Paranjape, Benson, and Leskovec (WSDM 2017) for temporal interaction streams; Kumar et al. (WWW 2018) for Reddit; additional citations appear on individual pages. |
| Network Repository: `ia-enron-email-dynamic`, `ia-reality-call`, `soc-wiki-elec`, `tech-as-topology`, `soc-youtube-growth` | [CC BY-SA policy](https://networkrepository.com/policy.php) (version not specified). Credit Rossi and Ahmed (AAAI 2015) and each dataset's cited creators; preserve notices, identify modifications, and distribute adapted data under the same license. |

Prepared streams are modified data: undirected conversion, 14-day edge lifetime
for timestamped inputs, cleaning, truncation, and format conversion as described
in [README.md](README.md). Preparation does not grant additional data rights;
where no license is stated, redistribution permission is not established here.

## Container dependencies

Ubuntu packages (including compilers, OpenJDK, Boost, Qt/qmake, Python, NumPy,
pandas, Matplotlib, and PyYAML) retain their individual licenses and dependency
notices; the installed texts are in `/usr/share/doc/<package>/copyright` inside
the image. Preserve applicable license texts and notices when redistributing
software, data, or the container.
