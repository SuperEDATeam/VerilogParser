from pulp import LpMinimize, LpProblem, LpVariable, lpSum, LpBinary, LpStatus, value  #这是 python 对应的求解器包，需要 pip 下载

import os,sys
class Node:
    def __init__(self, name, duration, scheduled=False, starttime=-1, pred=None, end=False,gates=None):
        self.name = name
        self.duration = duration  #持续时间
        self.scheduled = scheduled #是否是已调度
        self.starttime = starttime  #开始时间，起始节点置为0，其余-1
        self.pred = pred if pred else []  #前置节点
        self.end = end  #是否是结束节点
        self.alap_starttime=9999  #ALAP算法得到的最晚开始时间
        self.gates=gates  #运算的门属性

#读取blif文件，创建nodes[],存放各初始化的node节点
def read_blif(inputfile,and_duration,or_duration,not_duration): 
    with open(inputfile, "r") as f:
        lines = f.readlines()

    nodes = []
    start_nodes = []
    end_nodes = []
    all_outputs = set()
    node_map = {}

    for i, line in enumerate(lines):
            if line.startswith(".inputs"):
                inputs = line.strip().split()[1:]
                for name in inputs:
                    node = Node(name, 1, True, 0)
                    nodes.append(node)
                    node_map[name] = node
                    start_nodes.append(node)
            elif line.startswith(".outputs"):
                outputs = line.strip().split()[1:]
                all_outputs.update(outputs)
            elif line.startswith(".names"):
                parts = line.strip().split()
                preds = parts[1:-1]
                name = parts[-1]
                if (i+2 <len(lines)) and (not lines[i+1].startswith("0")) and (not lines[i+2].startswith(".names")) and (not lines[i+2].startswith(".end")):
                        node = Node(name, or_duration, False, -1, preds, name in all_outputs,"or")
                elif((i+1 <len(lines)) and (lines[i+1].startswith("0"))):
                    node = Node(name, not_duration, False, -1, preds, name in all_outputs,"not")
                else:
                    node = Node(name, and_duration, False, -1, preds, name in all_outputs,"and")
                nodes.append(node)
                node_map[name] = node
                if node.end:
                    end_nodes.append(node)
    
    return nodes, end_nodes, node_map,start_nodes

# ILP 模型构建，此步是 AIGC，返回model，也就是对应的上课所写的要求输入串
def build_ilp_model(nodes, and_limit, or_limit, not_limit):
    max_cycle = 10
    model = LpProblem("ML-RCS-Scheduling", LpMinimize)
    x = {
        node.name: {
            t: LpVariable(f"x_{node.name}_{t}", 0, 1, LpBinary)
            for t in range(1, max_cycle + 1)
        }
        for node in nodes if node.gates
    }
    start_time = {
        node.name: LpVariable(f"start_{node.name}", 1, max_cycle, cat="Integer")
        for node in nodes if node.gates
    }
    for node in nodes:
        if node.gates:
            model += lpSum(x[node.name][t] for t in range(1, max_cycle + 1)) == 1
            model += start_time[node.name] == lpSum(t * x[node.name][t] for t in range(1, max_cycle + 1))
    for node in nodes:
        if node.gates:
            for pred in node.pred:
                pred_node = next((n for n in nodes if n.name == pred), None)
                if pred_node and pred_node.gates:
                    model += start_time[node.name] >= start_time[pred_node.name] + pred_node.duration
    for t in range(1, max_cycle + 1):
        model += lpSum(x[node.name][t] for node in nodes if node.gates == "and") <= and_limit
        model += lpSum(x[node.name][t] for node in nodes if node.gates == "or") <= or_limit
        model += lpSum(x[node.name][t] for node in nodes if node.gates == "not") <= not_limit
    end_nodes = [node for node in nodes if node.end and node.gates]
    if end_nodes:
        latest_time = LpVariable("latest_time", 1, max_cycle, cat="Integer")
        for node in end_nodes:
            model += latest_time >= start_time[node.name]
        model += latest_time
    else:
        model += lpSum(start_time[node.name] for node in nodes if node.gates)
    return model, start_time


if __name__ == "__main__":

    inputfile = "..\output\output.blif"

    try:
        and_duration = int(input("请输入 AND 门的耗时: "))
        or_duration = int(input("请输入 OR 门的耗时: "))
        not_duration = int(input("请输入 NOT 门的耗时: "))
    except ValueError:
        print("输入格式错误，请输入整数")
        sys.exit(1)

    print("\n[阶段1] 正在读取 BLIF 并构建节点...")
    nodes, _, _, _ = read_blif(inputfile, and_duration, or_duration, not_duration)

    try:
        and_limit = int(input("请输入 AND 门的资源上限: "))
        or_limit = int(input("请输入 OR 门的资源上限: "))
        not_limit = int(input("请输入 NOT 门的资源上限: "))
    except ValueError:
        print("资源限制必须为整数")
        sys.exit(1)

    choice = input("是否继续进行 ILP 调度？(y/n): ").strip().lower()
    if choice != 'y':
        print("操作已取消，程序终止。")
        sys.exit(1)

    print("[阶段2] 正在构建 ILP 模型...")
    model, start_time = build_ilp_model(nodes, and_limit, or_limit, not_limit)

    print("[阶段3] 求解中...")
    status = model.solve()
    print(f"ILP 求解状态: {LpStatus[status]}")

    if LpStatus[status] != "Optimal":
        print("调度失败：未找到最优解")
        sys.exit(2)

    print("[阶段4] 调度结果如下：")
    for node in nodes:
        if node.gates:
            st = value(start_time[node.name])
            if st is not None:
                print(f"{node.name}：Cycle {int(round(st))} （类型: {node.gates}, 耗时: {node.duration}）")  #弹错原因： st未在程序运行前赋给特定类型，导致python误报

    sys.exit(0)