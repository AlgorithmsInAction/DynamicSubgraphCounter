# License and third-party notices

Except where a file states otherwise, SubgraphCounter is distributed under the
GNU GPL version 3; the full text is in [LICENSE](LICENSE). Source files that
offer "version 3 or later" retain that option. Third-party components retain
their own copyright and license notices.

| Component | Version | License and notice location |
| --- | --- | --- |
| AlgoraCore | v1.4 | GPLv3; `COPYING` and `LICENSE` inside `deps/algora-source.zip` |
| AlgoraDyn | v1.2 | GPLv3; `COPYING` and `LICENSE` inside `deps/algora-source.zip` |
| oaqc (OB baseline) | v2.0.0 | GPL version 3 or later; declared in `deps/oaqc/DESCRIPTION` after fetching |
| ESCAPE | `7ec2f93c524a0b47cdc8c639602e90d1c1fceff3` plus the local memory-leak patch | Apache License 2.0; see [license text](licenses/Apache-2.0.txt) and the upstream `LICENSE` in the fetched `deps/escape/` checkout |
| CLI11 single header | v2.1.2 | BSD 3-Clause; [copyright and license notice](licenses/CLI11-BSD-3-Clause.txt), also retained at the top of `src/io/CLI11.hpp` |

The OB and ESCAPE sources are fetched during the default build; they are not
stored in this repository. Copyright notices in their source files remain in
effect. Distributions of the built program must also satisfy the applicable
third-party license requirements.
