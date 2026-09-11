import sys
import networkx as nx
import matplotlib.pyplot as plt


def read_graph_until(filename, timestep, directed=False):
    """Reads edges up to a given timestep and returns the graph + appearance order + last edge."""
    G = nx.DiGraph() if directed else nx.Graph()
    appearance_order = {}
    current_index = 0
    edge_change = None

    with open(filename, 'r') as f:
        time = 0
        for i, line in enumerate(f):
            if time >= timestep:
                break

            parts = line.strip().split()
            if len(parts) < 3 or parts[0] == "#":
                continue

            u, v = map(int, parts[:2])
            sign = parts[2]

            # Record appearance order
            for node in (u, v):
                if node not in appearance_order:
                    appearance_order[node] = current_index
                    current_index += 1

            # Add or remove edges
            if sign == "+1":
                G.add_edge(u, v)
            elif sign == "-1":
                if G.has_edge(u, v):
                    G.remove_edge(u, v)

            edge_change = (u, v, sign)
            time += 1

    return G, appearance_order, edge_change


def two_hop_neighborhood(G, node):
    """Returns the 2-hop neighborhood (node itself, its neighbors, and neighbors of neighbors)."""
    if node not in G:
        return set()

    # For directed graphs, consider both predecessors and successors
    neighbors = set(G.predecessors(node)).union(set(G.successors(node))) if G.is_directed() else set(G.neighbors(node))
    two_hop = set()
    for n in neighbors:
        if G.is_directed():
            two_hop.update(G.predecessors(n))
            two_hop.update(G.successors(n))
        else:
            two_hop.update(G.neighbors(n))

    return neighbors.union({node})


def draw_graph_timestep(G, appearance_order, edge_change, timestep, mode="all", focus_nodes=None, highlighted_ids=None):
    """Draws the full or 2-hop network at a given timestep, with curved reciprocal edges for directed graphs."""
    u, v, sign = edge_change if edge_change else (None, None, None)
    plt.figure(figsize=(8, 6))

    inverse_order = {v: k for k, v in appearance_order.items()}
    marked_nodes = []
    # Determine subgraph
    match mode:

      case "2hop":
        v1, v2 = focus_nodes
        # v1 = inverse_order[v1]
        # v2 = inverse_order[v2]
        n1 = two_hop_neighborhood(G, v1)
        n2 = two_hop_neighborhood(G, v2)
        combined = n1.union(n2)
        subG = G.subgraph(combined).copy()
        title = f"2-Hop Neighborhoods of {v1} and {v2} at Timestep {timestep}"
      case "s5":
            v1, v2 = focus_nodes
            # v1 = inverse_order[v1]
            # v2 = inverse_order[v2]
            sub_nodes, marked_nodes = s5_nodes(G, v1, v2, appearance_order, highlighted_ids)
            subG = G.subgraph(sub_nodes).copy()
            title = f"S5 Subgraph for nodes {v1} and {v2} at Timestep {timestep}"
      case "4C":
            v1, v2 = focus_nodes
            sub_nodes = fCycle_nodes(G, v1, v2, appearance_order, highlighted_ids)
            subG = G.subgraph(sub_nodes).copy()
            title = f"S5 Subgraph for nodes {v1} and {v2} at Timestep {timestep}"
      case _:
            subG = G
            title = f"Full {'Directed' if G.is_directed() else 'Undirected'} Network at Timestep {timestep}"

    pos = nx.spring_layout(subG, seed=42)
    # pos = nx.bfs_layout(subG, start=108)

    # Highlighted nodes (by internal ID)
    highlighted_nodes = set()
    if highlighted_ids:
        highlighted_nodes = {inverse_order[i] for i in highlighted_ids if i in inverse_order and inverse_order[i] in subG}

    # Draw all nodes
    nx.draw_networkx_nodes(
        subG,
        pos,
        nodelist=[n for n in subG.nodes() if n not in highlighted_nodes],
        node_color="#dddddd",
        edgecolors="black",
        linewidths=1.5,
        node_size=700
    )

    # Draw highlighted nodes
    if highlighted_nodes:
        nx.draw_networkx_nodes(
            subG,
            pos,
            nodelist=list(highlighted_nodes),
            node_color="#ffffff",
            edgecolors="red",
            linewidths=3,
            node_size=800
        )

    if marked_nodes:
        nx.draw_networkx_nodes(
            subG,
            pos,
            nodelist=list(marked_nodes),
            node_color="#e0ac4cff",
            edgecolors="black",
            linewidths=1.5,
            node_size=700
        )

    # --- Draw Edges ---
    if subG.is_directed():
        # Identify reciprocal (bidirectional) edges
        curved_edges = [(u, v) for u, v in subG.edges() if subG.has_edge(v, u)]
        straight_edges = [e for e in subG.edges() if e not in curved_edges and (e[1], e[0]) not in curved_edges]

        # Draw straight edges
        nx.draw_networkx_edges(
            subG, pos, edgelist=straight_edges, edge_color="gray", alpha=0.7,
            arrows=True, arrowsize=20
        )

        # Draw curved edges (bidirectional)
        nx.draw_networkx_edges(
            subG, pos, edgelist=curved_edges, edge_color="gray", alpha=0.7,
            arrows=True, arrowsize=20, connectionstyle="arc3,rad=0.15"
        )
    else:
        # Undirected edges (normal)
        nx.draw_networkx_edges(subG, pos, edge_color="gray", alpha=0.7)

    # --- Highlight the changed edge ---
    if u in subG and v in subG:
        style = "solid" if sign == "+1" else "dashed"
        color = "green" if sign == "+1" else "red"
        connection = "arc3"

        if subG.is_directed() and subG.has_edge(v, u):  # if reciprocal exists, curve it
            connection = "arc3,rad=0.15"

        nx.draw_networkx_edges(
            subG,
            pos,
            edgelist=[(u, v)],
            edge_color=color,
            style=style,
            width=3,
            arrows=subG.is_directed(),
            arrowsize=25,
            connectionstyle=connection
        )

    # Labels
    labels = {n: f"{n}\n({appearance_order.get(n, '?')})" for n in subG.nodes()}
    nx.draw_networkx_labels(subG, pos, labels=labels, font_size=9)

    plt.title(title)
    plt.axis("off")
    plt.tight_layout()
    plt.show()

def s5_nodes(G, v1, v2, internal_mapping, high_nodes):
    """Return nodes X connected to both v1 and v2 and their neighbors connected to at least one input node.
       Returns: subgraph nodes set, set of nodes that are neighbors of both input nodes."""
    
    if G.is_directed():
        neighbors_v1 = set(G.predecessors(v1)).union(G.successors(v1))
        neighbors_v2 = set(G.predecessors(v2)).union(G.successors(v2))
    else:
        neighbors_v1 = set(G.neighbors(v1))
        neighbors_v2 = set(G.neighbors(v2))
    
    both_neighbors = neighbors_v1.intersection(neighbors_v2)  # neighbors of both v1 and v2
    result_nodes = set()
    marked_nodes = set()
    for x in both_neighbors:
        if internal_mapping[x] in high_nodes:
            continue
        # neighbors of x
        if G.is_directed():
            nx_neighbors = set(G.predecessors(x)).union(G.successors(x))
        else:
            nx_neighbors = set(G.neighbors(x))
        
        # include x only if all neighbors connect to at least one input node
        for n in nx_neighbors:
            if internal_mapping[n] not in high_nodes and (n in neighbors_v1 or n in neighbors_v2) :
                result_nodes.add(x)  # always include x if any neighbor satisfies condition
                result_nodes.add(n)  # include the neighbor
                marked_nodes.add(x)
        
    result_nodes.add(v1)
    result_nodes.add(v2)

    return result_nodes, marked_nodes


def fCycle_nodes(G, v1, v2, internal_mapping, high_nodes):
    """
    Return nodes forming a 4-cycle containing v1 and v2.

    Possible shapes:
        1) v1 - x - v2 - y - v1
        2) v1 - v2 - x - y - v1
    """

    def neighbors(v):
        if G.is_directed():
            return set(G.predecessors(v)).union(G.successors(v))
        return set(G.neighbors(v))

    neighbors_v1 = neighbors(v1)
    neighbors_v2 = neighbors(v2)

    result_nodes = set()

    # -------------------------
    # Case 1: v1 - x - v2 - y - v1
    # -------------------------
    common = neighbors_v1 & neighbors_v2
    valid_common = [x for x in common if internal_mapping[x] not in high_nodes]

    for i in range(len(valid_common)):
        for j in range(i + 1, len(valid_common)):
            x = valid_common[i]
            y = valid_common[j]
            if G.has_edge(x, y) or G.has_edge(y, x):
                result_nodes.update({v1, v2, x, y})

    # -------------------------
    # Case 2: v1 - v2 - x - y - v1
    # -------------------------
    if G.has_edge(v1, v2) or G.has_edge(v2, v1):

        valid_v2_neighbors = [
            x for x in neighbors_v2
            if x != v1 and internal_mapping[x] not in high_nodes
        ]

        valid_v1_neighbors = [
            y for y in neighbors_v1
            if y != v2 and internal_mapping[y] not in high_nodes
        ]

        for x in valid_v2_neighbors:
            for y in valid_v1_neighbors:
                if x != y and (G.has_edge(x, y) or G.has_edge(y, x)):
                    result_nodes.update({v1, v2, x, y})

    return result_nodes


def main():
    if len(sys.argv) < 4:
        print("Usage:")
        print("  python visualize_graph.py <input_file> <timestep> --all [--d] [--highlight id1 id2 ...]")
        print("  python visualize_graph.py <input_file> <timestep> --2hop <v1> <v2> [--d] [--highlight id1 id2 ...]")
        sys.exit(1)

    filename = sys.argv[1]
    timestep = int(sys.argv[2])
    mode = sys.argv[3].lstrip("-")

    v1 = v2 = None
    highlighted_ids = []
    directed = "--d" in sys.argv

    # Read graph up to timestep
    G, appearance_order, edge_change = read_graph_until(filename, timestep, directed=directed)

    # Parse remaining args
    args = sys.argv[4:]
    if mode in ["2hop", "s5", "4C"] :
        if len(args) < 2:
            # print(f"Error: --{mode} mode requires two vertex IDs.")
            # sys.exit(1)
            v1, v2, sign = edge_change
        else:  
            v1 = int(args[0])
            v2 = int(args[1])
            args = args[2:]

    # Optional --highlight
    if "--highlight" in args:
        idx = args.index("--highlight")
        highlighted_ids = list(map(int, args[idx + 1:]))


    # If 2-hop mode, print degrees
    # if mode == "2hop":
    #     print_degrees(G, v1, v2, appearance_order)
        
    # Draw
    draw_graph_timestep(
        G,
        appearance_order,
        edge_change,
        timestep,
        mode=mode,
        focus_nodes=(v1, v2) if v1 is not None else None,
        highlighted_ids=highlighted_ids
    )


def print_degrees(G, v1, v2, appearance_order):
    """Print degrees for v1, v2, and their 1-hop neighbors."""
    print("\n=== Degree Report (1-hop Neighborhoods) ===")

    inverse_order = {v: k for k, v in appearance_order.items()}
    v1 = inverse_order[v1]
    v2 = inverse_order[v2]

    def one_hop(G, node):
        if G.is_directed():
            return set(G.predecessors(node)).union(G.successors(node))
        return set(G.neighbors(node))

    for v in (v1, v2):
        print(f"\nNode {v}:")
        print(f"  Degree: {G.degree(v)}")

        neighbors = one_hop(G, v)
        print(f"  1-hop neighbors: {sorted(neighbors)}")

        for n in sorted(neighbors):
            if G.degree(n) -1 > 0:
                print(f"    - Node {n}: degree {G.degree(n) -1}")


if __name__ == "__main__":
    main()
