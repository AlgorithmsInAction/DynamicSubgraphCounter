# License and third-party notices

Except where a file states otherwise, SubgraphCounter is distributed under the
GNU GPL version 3; the full text is in [LICENSE](LICENSE). Source files that
offer "version 3 or later" retain that option. Third-party components retain
their own copyright and license notices.

| Component | Version | License and notice location |
| --- | --- | --- |
| AlgoraCore | v1.4 | GPLv3; `COPYING` and `LICENSE` inside `deps/algora-source.zip` |
| AlgoraDyn | v1.2 | GPLv3; `COPYING` and `LICENSE` inside `deps/algora-source.zip` |
| oaqc (OB baseline) | v2.0.0 | GPL version 3 or later; see the upstream [license declaration](https://github.com/schochastics/oaqc/blob/v2.0.0/DESCRIPTION) and [license text](https://github.com/schochastics/oaqc/blob/v2.0.0/LICENSE.md) |
| ESCAPE | `7ec2f93c524a0b47cdc8c639602e90d1c1fceff3` plus the local memory-leak patch | Apache License 2.0; see the upstream [LICENSE](https://bitbucket.org/seshadhri/escape/src/7ec2f93c524a0b47cdc8c639602e90d1c1fceff3/LICENSE) |
| CLI11 single header | v2.1.2 | BSD 3-Clause; [copyright and license notice](src/io/CLI11.hpp) at the top of the header |
| Memory usage utilities (`memory.h`) | — | Creative Commons Attribution 3.0 Unported (CC BY 3.0); by David Robert Nadeau ([NadeauSoftware.com](http://NadeauSoftware.com/)); see the [notice in the header](src/util/memory.h) and [license](https://creativecommons.org/licenses/by/3.0/) |

The OB and ESCAPE sources are fetched during the default build; they are not
stored in this repository. Copyright notices in their source files remain in
effect. Distributions of the built program must also satisfy the applicable
third-party license requirements.
