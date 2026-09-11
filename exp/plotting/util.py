import yaml
import matplotlib.pyplot as plt
import os
import numpy as np
from matplotlib.ticker import LogFormatterSciNotation


class ComputerModernLogFormatter(LogFormatterSciNotation):
    """Render exponents with math fonts, not cmss10's incomplete text encoding."""

    def __call__(self, x, pos=None):
        return super().__call__(x, pos).replace(r"\mathdefault", r"\mathrm")


import matplotlib
matplotlib.use("Agg")

# matplotlib.use('tkagg')

# Match the paper's Computer Modern sans-serif look without invoking an
# external LaTeX process. Matplotlib bundles these fonts with its installation,
# and fonttype 42 keeps text embedded as vector TrueType glyphs in the PDF.
plt.rcParams.update({
    "text.usetex": False,
    "ps.useafm": False,
    "pdf.fonttype": 42,
    "ps.fonttype": 42,
    "font.family": "sans-serif",
    "font.sans-serif": ["cmss10", "DejaVu Sans"],
    "axes.unicode_minus": False,
    "font.weight": "bold",
    "axes.labelweight": "bold",
    "axes.titleweight": "bold",
    "mathtext.fontset": "cm",
    "font.size": 24,
    "lines.linewidth":4,
    "lines.markersize":10,
})



# Load YAML config
BASE_DIR = os.path.dirname(__file__)

with open(os.path.join(BASE_DIR, 'config.yaml'), 'r') as f:
    config = yaml.safe_load(f)

graph_names = config.get("graph_names", {})
partition_config = config.get("partition_config", {})
algo_config = config.get("algo_config", {})
metric_config = config.get("metric_config", {})
pattern_config = config.get("pattern_config", {})

# Extract styling from config
palette = {k : v["color"] for k, v in algo_config.items()}
algo_display = {k : v["display_name"] for k, v in algo_config.items()}
algo_hatches = {k : v["pattern"] for k, v in algo_config.items()}
partition_display = {k : v["display_name"] for k, v in partition_config.items()}
partition_color = {k: v["color"] for k, v in partition_config.items()}
partition_marker = {k: v["marker"] for k, v in partition_config.items()}
pattern_hatches = {k: v["pattern"] for k, v in partition_config.items()}
partition_linestyle = {k: v["linestyle"] for k, v in partition_config.items()}

metric_palette = {k: v["color"] for k, v in metric_config.items()}
metric_markers = {k: v["marker"] for k, v in metric_config.items()}
metric_linestyle = {k: v["linestyle"] for k, v in metric_config.items()}


pattern_palette = {k: v["color"] for k, v in pattern_config.items()}
pattern_markers = {k: v["marker"] for k, v in pattern_config.items()}
pattern_linestyle = {k: v["linestyle"] for k, v in pattern_config.items()}
pattern_display = {k: v["display_name"] for k, v in pattern_config.items()}


graphs = {"g_flickr","g_as-newman","g_bio-proteins","g_cit-hep-ph","g_blog-nat06all","g_epinions","c_atp-gr-qc","c_answers","c_delicious","c_web-notredame","c_as-routeviews","c_ca-dblp","simple_wiki","nl_wiki","pl_wiki","it_wiki","fr_wiki","de_wiki","sx-superuser_f","sx-askubuntu_f","wiki-talk-temporal_f","ia-enron-email-dynamic_f","sx-stackoverflow_f","soc-youtube-growth_f"}
def get_label(algo,part):
    if part == 'none':
        return f"{algo_display[algo]}"   
    else:
        return f"{algo_display[algo]} | {partition_display[part]}"

def geo_mean(x):
        x = x[x > 0]
        return np.exp(np.log(x).mean())
def geo_std(x):
    x = x[x > 0]
    return np.exp(np.log(x).std())

def apply_graph_groups(df, graph_split=True, build_med=False):
    graph_groups_dict = {}
    def assign_graph_group(g):
        if not graph_split:
            group_name = g
        elif g.startswith("g_"):
            group_name = "KRONg"
        elif g.startswith("c_"):
            group_name = "KRONc"
        elif "wiki" in g and "talk" not in g and "elec" not in g:
            group_name = "Wiki"
        else:
            group_name = "TMP"
        # Build mapping
        if group_name not in graph_groups_dict:
            graph_groups_dict[group_name] = set()
        graph_groups_dict[group_name].add(g)
        return group_name
    
    df = df.copy()
    df["graph_group"] = df["graph"].apply(assign_graph_group)
    return df

kron_g= "KRONg"
kron_c= "KRONc"
wiki= "Wiki"
temp= "TMP"


graph_info = {
    # Wikipedia Hyperlink Networks
    "simple_wiki": (wiki, "A"),
    "nl_wiki": (wiki, "B"),
    "pl_wiki": (wiki, "C"),
    "it_wiki": (wiki, "D"),
    "fr_wiki": (wiki, "E"),
    "de_wiki": (wiki, "F"),

    # Temporal Graphs
    "reality-call": (temp, "A"),
    "wiki-elec": (temp, "B"),
    "tech-as-topology": (temp, "C"),
    "reddit-body": (temp, "D"),
    "mathoverflow": (temp, "E"),
    "enron": (temp, "F"),
    "reddit-title": (temp, "G"),
    "askubuntu": (temp, "H"),
    "superuser": (temp, "I"),
    "wiki-talk": (temp, "J"),
    "youtube-growth": (temp, "K"),
    "stackoverflow": (temp, "L"),

    # Constant-Size Kronecker Graphs
    "c_answers": (kron_c, "A"),
    "c_delicious": (kron_c, "B"),
    "c_epinions": (kron_c, "C"),
    "c_flickr": (kron_c, "D"),
    "c_blog-nat05-6m": (kron_c, "E"),
    "c_blog-nat06all": (kron_c, "F"),
    "c_cit-hep-ph": (kron_c, "G"),
    "c_cit-hep-th": (kron_c, "H"),
    "c_ca-dblp": (kron_c, "I"),
    "c_ca-gr-qc": (kron_c, "J"),
    "c_ca-hep-th": (kron_c, "K"),
    "c_ca-hep-ph": (kron_c, "L"),
    "c_web-notredame": (kron_c, "M"),
    "c_as-newman": (kron_c, "N"),
    "c_as-routeviews": (kron_c, "O"),
    "c_gnutella-30": (kron_c, "P"),
    "c_gnutella-25": (kron_c, "Q"),
    "c_atp-gr-qc": (kron_c, "R"),
    "c_bio-proteins": (kron_c, "S"),

    # Growing Kronecker Graphs
    "g_answers": (kron_g, "A"),
    "g_delicious": (kron_g, "B"),
    "g_epinions": (kron_g, "C"),
    "g_flickr": (kron_g, "D"),
    "g_blog-nat05-6m": (kron_g, "E"),
    "g_blog-nat06all": (kron_g, "F"),
    "g_cit-hep-ph": (kron_g, "G"),
    "g_cit-hep-th": (kron_g, "H"),
    "g_ca-dblp": (kron_g, "I"),
    "g_ca-gr-qc": (kron_g, "J"),
    "g_ca-hep-ph": (kron_g, "K"),
    "g_ca-hep-th": (kron_g, "L"),
    "g_web-notredame": (kron_g, "M"),
    "g_as-newman": (kron_g, "N"),
    "g_as-routeviews": (kron_g, "O"),
    "g_gnutella-25": (kron_g, "P"),
    "g_gnutella-30": (kron_g, "Q"),
    "g_atp-gr-qc": (kron_g, "R"),
    "g_bio-proteins": (kron_g, "S"),
}
