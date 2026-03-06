from collections import defaultdict
from typing import List, Tuple
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
def read_blif(inputfile,node_duration): 
    with open(inputfile, "r") as f:
        lines = f.readlines()

    nodes = []
    start_nodes = []
    end_nodes = []
    all_outputs = set()
    node_map = {}
    node_label = ["and","or","not"]
    temp_label=-1

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
                # or 门判断
                if (
                    (i + 2 < len(lines)) and
                    (not lines[i + 1].startswith(".names")) and
                    (not lines[i + 2].startswith(".names")) and
                    (not lines[i + 2].startswith(".end"))
                ):  
                    temp_label=1
                # not 门判断
                elif((i+1 <len(lines)) and (lines[i+1].startswith("0"))):
                    temp_label=2
                # and 门判断
                else:
                    temp_label=0
                if name in node_map:
                    # 已存在该节点，合并 pred（不重复）
                    existing_node = node_map[name]
                    existing_node.pred = list(set(existing_node.pred + preds))
                else:
                    # 创建新节点
                    node = Node(name, node_duration[temp_label], False, -1, preds, name in all_outputs, node_label[temp_label]) 
                    nodes.append(node)
                    node_map[name] = node
                    if node.end:
                        end_nodes.append(node)
    
    return nodes, end_nodes, node_map,start_nodes

#ASAP 算法判断的可行的下一节点函数，成功返回对应节点
def find_next_node(unvisited_nodes, node_map):
    for node in unvisited_nodes:
        if not node.scheduled:
            all_pred_scheduled = True
            pred_nodes = []
            for pred_name in node.pred:
                pred_node = node_map.get(pred_name)
                if pred_node:
                    pred_nodes.append(pred_node)
                    if not pred_node.scheduled:
                        all_pred_scheduled = False
                        break
            if all_pred_scheduled:
                return node, pred_nodes
    return None, []

# ASAP 调度算法 
def asap_schedule():
    #ASAP调度算法
    while True:
        if all(node.scheduled for node in endnodes):
            break
        node_to_schedule, pred_nodes = find_next_node(unvisited_nodes, node_map)
        if node_to_schedule:
            schedule_node(node_to_schedule, pred_nodes)
            unvisited_nodes.remove(node_to_schedule)  # 删除已调度节点
        else:
            break  # 没有可调度的节点，可能存在循环依赖

# ASAP 对应的将得到的节点进行调度的函数
def schedule_node(node, pred_nodes):
    pred_finish_time = [n.starttime + n.duration for n in pred_nodes]
    node.starttime = max(pred_finish_time) if pred_finish_time else 0
    node.scheduled = True

# MR-LCS 算法对应的将得到的节点进行调度的函数
def schedule_node_mrlcs(node, current_cycle):
    node.starttime = current_cycle
    node.scheduled = True

# ALAP 调度算法
def alap_schedule(nodes, max_time):
    # Step 1: 初始化所有节点的 alap_starttime 为 max_time（保守值）
    for node in nodes:
        node.alap_starttime = max_time

    # Step 2: 反向拓扑顺序调度
    sorted_nodes = sorted(nodes, key=lambda n: n.starttime, reverse=True)

    for node in sorted_nodes:
        if node.end:
            node.alap_starttime = max_time - node.duration
        else:
            # 查找所有后继节点
            successors = [n for n in nodes if node.name in n.pred]
            if successors:
                min_successor_alap = min(s.alap_starttime for s in successors)
                node.alap_starttime = min_successor_alap - node.duration
            else:
                node.alap_starttime = max_time - node.duration  # 无后继，处理孤立节点

# MR-LCS 调度算法，基于已有的ALAP得到 slack,所以是在 ALAP调度后重新调度一遍全部节点
def mr_lcs_schedule_precise(nodes: List[Node], node_map: dict, latency_limit: int) -> Tuple[dict, dict]:
    gate_types = set(n.gates for n in nodes if n.gates)
    a = {gate: 1 for gate in gate_types}  # 初始每种资源上限为1
    resource_usage = {gate: defaultdict(int) for gate in gate_types}
    l = 1  # 当前调度周期

    def is_ready(node, current_time):
        if node.scheduled:
            return False
        return all(
            pred.scheduled and (pred.starttime + pred.duration <= current_time)
            for pred in (node_map[p] for p in node.pred if p in node_map)
        )

    def can_schedule(node, current_cycle):
        for t in range(current_cycle, current_cycle + node.duration):
            if resource_usage[node.gates][t] >= a[node.gates]:
                return False
        return True

    def update_resource_usage(node, current_cycle):
        for t in range(current_cycle, current_cycle + node.duration):
            resource_usage[node.gates][t] += 1

    while not all(n.scheduled for n in nodes if n.end):
        for gate_type in gate_types:
            Ulk = [n for n in nodes if n.gates == gate_type and is_ready(n, l)]
            Ulk.sort(key=lambda n: n.alap_starttime - l)  # 按 slack 升序排

            for node in Ulk:
                slack = node.alap_starttime - l
                if can_schedule(node, l):
                    node.starttime = l
                    node.scheduled = True
                    update_resource_usage(node, l)
                elif slack == 0:
                    # 为调度 slack=0 的节点而提升资源数
                    for t in range(l, l + node.duration):
                        resource_usage[gate_type][t] += 1
                        a[gate_type] = max(a[gate_type], resource_usage[gate_type][t])
                    node.starttime = l
                    node.scheduled = True
        l += 1

    result_schedule = {n.name: n.starttime for n in nodes}
    return result_schedule, a


### 主调度逻辑
# step 1:为每个门的持续时间（延迟）赋值
node_duration=[]
node_duration.append(int(input("请输入and门的耗时:")))
node_duration.append(int(input("请输入or门的耗时:")))
node_duration.append(int(input("请输入not门的耗时:")))

#step 2:调度 read_blif 函数，返回依次为 node构成的列表、终止节点、nodemap(字典，键值对为 node名字——node存储体，便于用名字快速查找)、开始节点(starttime=0)
nodes, endnodes, node_map,start_nodes = read_blif("D:\\verlog\\2_Verilog编译器\\demo2_电路网表图绘制\\demo2_电路网表图绘制\\output\\output.blif",node_duration)

#step 3:ASAP调度，确定最早完成时间
unvisited_nodes = [n for n in nodes if not n.scheduled]
asap_schedule()
max_asap_end_time = max(n.starttime+n.duration-1 for n in endnodes)
print()
n = int(input("请输入结束时间: "))
if n < max_asap_end_time:
    print(f"[错误] 你输入的结束时间 {n} 小于 ASAP 所需的最小结束时间 {max_asap_end_time}，调度无法完成！")
    exit(1)

#step 4:ALAP 调度
alap_schedule(nodes,n+1)

#step 5:将ALAP调度后各节点均访问 scheduled=True ,改为未访问状态（开始节点除外）
for i in nodes:
    if i not in start_nodes:
        i.scheduled=False

#step 6:MR-LCS调度及打印
schedule_result, resource_limits = mr_lcs_schedule_precise(nodes, node_map, latency_limit=n+1)
print("\n[MR-LCS 调度结果]")
for name, time in sorted(schedule_result.items(), key=lambda x: x[1]):
    if time==0: continue
    print(f"Node {name} -> Start at Cycle {time}")
print("\n[每种门类型的最小资源需求]")
for gate, count in resource_limits.items():
    print(f"{gate.upper()} 门: {count}")