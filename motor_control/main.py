from graphviz import Digraph

# Create a mind map using Graphviz Digraph
dot = Digraph(comment="Business Economics and Financial Analysis - Summary")

# Main topic
dot.node("A", "Business Economics & Financial Analysis")

# Subtopics
subtopics = [
    ("B1", "Intro to Business Economics"),
    ("B2", "Theory of Firm"),
    ("B3", "Business Cycle"),
    ("B4", "Inflation"),
    ("B5", "Money Supply & Inflation Control"),
    ("B6", "National Income"),
    ("B7", "Types of Business Entities"),
    ("B8", "Joint-Stock, Cooperatives, Public Enterprises"),
    ("B9", "Sources of Finance"),
    ("B10", "Role of Business Economist")
]

for code, label in subtopics:
    dot.node(code, label)
    dot.edge("A", code)

# Add key points for each subtopic
details = {
    "B1": ["Business = Profit activity", "Types: Economic / Non-Economic", "Economics: Micro & Macro", "Business Economics = Apply eco. to decisions"],
    "B2": ["Profit maximization", "Managerial theories (Baumol, Marris, Williamson)", "Behavioral theories (Simon, Cyert & March)"],
    "B3": ["Phases: Depression → Recovery → Expansion → Boom → Recession"],
    "B4": ["Types: Creeping, Galloping, Hyper, Suppressed, Hidden, Deflation", "Causes: Demand-pull, Cost-push, Built-in, Low supply"],
    "B5": ["Instruments: Bank Rate, OMO, CRR, SLR, Credit Rationing, Moral Suasion, Direct Action, Margins, Deficit financing"],
    "B6": ["GDP, GNP, NNP, NI, PI, DI, PCI", "Methods: Income, Product, Expenditure", "Difficulties: Data, welfare not measured"],
    "B7": ["Sole Trader: Easy but risky", "Partnership: Shared resources, unlimited liability"],
    "B8": ["Joint-Stock: Large, limited liability", "Cooperatives: Mutual help, govt support", "Public enterprises: Govt-controlled"],
    "B9": ["Conventional: Shares, debentures, loans", "Non-Conventional: Hire, leasing, venture, factoring"],
    "B10": ["Guide management decisions", "Forecast, monitor policies", "Plan profits & investments"]
}

# Add detail nodes
for parent, points in details.items():
    for i, point in enumerate(points):
        node_id = f"{parent}_{i}"
        dot.node(node_id, point, shape="note", style="filled", color="lightgrey")
        dot.edge(parent, node_id)

# Save and render
output_path = "/mnt/data/business_economics_summary"
dot.render(output_path, format="png", cleanup=True)

output_path + ".png"
